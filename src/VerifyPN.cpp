/* TAPAAL untimed verification engine verifypn 
 * Copyright (C) 2011-2018  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>,
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * CTL Extension
 *                          Peter Fogh <peter.f1992@gmail.com>
 *                          Isabella Kaufmann <bellakaufmann93@gmail.com>
 *                          Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>
 *                          Lasse Steen Jensen <lassjen88@gmail.com>
 *                          Søren Moss Nielsen <soren_moss@mac.com>
 *                          Samuel Pastva <daemontus@gmail.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * Stubborn sets, query simplification, siphon-trap property
 *                          Frederik Meyer Boenneland <sadpantz@gmail.com>
 *                          Jakob Dyhr <jakobdyhr@gmail.com>
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *                          Mads Johannsen <mads_johannsen@yahoo.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <utility>
#include <functional>
#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#endif

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/QueryBinaryParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"

using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

ReturnValue contextAnalysis(ColoredPetriNetBuilder& cpnBuilder, PetriNetBuilder& builder, const PetriNet* net, std::vector<std::shared_ptr<Condition> >& queries)
{
    //Context analysis
    ColoredAnalysisContext context(builder.getPlaceNames(), builder.getTransitionNames(), net, cpnBuilder.getUnfoldedPlaceNames(), cpnBuilder.getUnfoldedTransitionNames(), cpnBuilder.isColored());
    for(auto& q : queries)
    {
        q->analyze(context);

        //Print errors if any
        if (context.errors().size() > 0) {
            for (size_t i = 0; i < context.errors().size(); i++) {
                fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
            }
            return ErrorCode;
        }
    }
    return ContinueCode;
}

std::vector<std::string> explode(std::string const & s)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, ','); )
    {
        result.push_back(std::move(token));
        if(result.back().empty()) result.pop_back();
    }

    return result;
}

ReturnValue parseOptions(int argc, char* argv[], options_t& options)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--k-bound") == 0) {  
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.kbound) != 1 || options.kbound < 0) {
                fprintf(stderr, "Argument Error: Invalid number of tokens \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0){
			if (i==argc-1) {
                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
				return ErrorCode;                           
            }
            char* s = argv[++i];
            if(strcmp(s, "BestFS") == 0)
				options.strategy = HEUR;
			else if(strcmp(s, "BFS") == 0)
				options.strategy = BFS;
			else if(strcmp(s, "DFS") == 0)
				options.strategy = DFS;
			else if(strcmp(s, "RDFS") == 0)
				options.strategy = RDFS;
			else if(strcmp(s, "OverApprox") == 0)
				options.strategy = OverApprox;
			else{
				fprintf(stderr, "Argument Error: Unrecognized search strategy \"%s\"\n", s);
				return ErrorCode;
			}
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query-reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.queryReductionTimeout) != 1 || options.queryReductionTimeout < 0) {
                fprintf(stderr, "Argument Error: Invalid query reduction timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lpsolve-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.lpsolveTimeout) != 1 || options.lpsolveTimeout < 0) {
                fprintf(stderr, "Argument Error: Invalid LPSolve timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--state-space-exploration") == 0) {
            options.statespaceexploration = true;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-statistics") == 0) {
            options.printstatistics = false;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            options.trace = true;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xml-queries") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            std::vector<std::string> q = explode(argv[++i]);
            for(auto& qn : q)
            {
                int32_t n;
                if(sscanf(qn.c_str(), "%d", &n) != 1 || n <= 0)
                {
                    std::cerr << "Error in query numbers : " << qn << std::endl;
                }
                else
                {
                    options.querynumbers.insert(--n);
                }
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.enablereduction) != 1 || options.enablereduction < 0 || options.enablereduction > 3) {
                fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if(options.enablereduction == 3)
            {
                options.reductions.clear();
                std::vector<std::string> q = explode(argv[++i]);
                for(auto& qn : q)
                {
                    int32_t n;
                    if(sscanf(qn.c_str(), "%d", &n) != 1 || n < 0 || n > 9)
                    {
                        std::cerr << "Error in reduction rule choice : " << qn << std::endl;
                        return ErrorCode;
                    }
                    else
                    {
                        options.reductions.push_back(n);
                    }
                }                
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--reduction-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.reductionTimeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid reduction timeout argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "--seed-offset") == 0) {
            if (sscanf(argv[++i], "%u", &options.seed_offset) != 1) {
                fprintf(stderr, "Argument Error: Invalid seed offset argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }  else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--partial-order-reduction") == 0) {
            options.stubbornreduction = false;
        } else if(strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--siphon-trap") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.siphontrapTimeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-trap timeout \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "--siphon-depth") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.siphonDepth) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-depth count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } 
        else if (strcmp(argv[i], "-tar") == 0)
        {
            options.tar = true;
            
        }
        else if (strcmp(argv[i], "--write-simplified") == 0)
        {
            options.query_out_file = std::string(argv[++i]);
        }
        else if(strcmp(argv[i], "--binary-query-io") == 0)
        {
            if (sscanf(argv[++i], "%u", &options.binary_query_io) != 1 || options.binary_query_io > 3) {
                fprintf(stderr, "Argument Error: Invalid binary-query-io value \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
        else if (strcmp(argv[i], "--write-reduced") == 0)
        {
            options.model_out_file = std::string(argv[++i]);
        }
#ifdef VERIFYPN_MC_Simplification
        else if (strcmp(argv[i], "-z") == 0)
        {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &options.cores) != 1) {
                fprintf(stderr, "Argument Error: Invalid cores count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
#endif
        else if (strcmp(argv[i], "-ctl") == 0){
            if(argc > i + 1){
                if(strcmp(argv[i + 1], "local") == 0){
                    options.ctlalgorithm = CTL::Local;
                }
                else if(strcmp(argv[i + 1], "czero") == 0){
                    options.ctlalgorithm = CTL::CZero;
                }
                else
                {
                    fprintf(stderr, "Argument Error: Invalid ctl-algorithm type \"%s\"\n", argv[i + 1]);
                    return ErrorCode;
                }
                i++;

            }
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--game-mode") == 0){
            options.gamemode = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpn-overapproximation") == 0) {
            options.cpnOverApprox = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: verifypn [options] model-file query-file\n"
                    "A tool for answering CTL and reachability queries of place cardinality\n" 
                    "deadlock and transition fireability for weighted P/T Petri nets\n" 
                    "extended with inhibitor arcs.\n"
                    "\n"
                    "Options:\n"
                    "  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
                    "  -t, --trace                        Provide XML-trace to stderr\n"
                    "  -s, --search-strategy <strategy>   Search strategy:\n"
                    "                                     - BestFS       Heuristic search (default)\n"
                    "                                     - BFS          Breadth first search\n"
                    "                                     - DFS          Depth first search (CTL default)\n"
                    "                                     - RDFS         Random depth first search\n"
                    "                                     - OverApprox   Linear Over Approx\n"
                    "  --seed-offset <number>             Extra noise to add to the seed of the random number generation\n"
                    "  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
                    "  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
                    "  -r, --reduction <type>             Change structural net reduction:\n"
                    "                                     - 0  disabled\n"
                    "                                     - 1  aggressive reduction (default)\n"
                    "                                     - 2  reduction preserving k-boundedness\n"
                    "                                     - 3  user defined reduction sequence, eg -r 3 0,1,2,3 to use rules A,B,C,D only, and in that order\n"
                    "  -d, --reduction-timeout <timeout>  Timeout for structural reductions in seconds (default 60)\n"
                    "  -q, --query-reduction <timeout>    Query reduction timeout in seconds (default 30)\n"
                    "                                     write -q 0 to disable query reduction\n"
                    "  -l, --lpsolve-timeout <timeout>    LPSolve timeout in seconds, default 10\n"
                    "  -p, --partial-order-reduction      Disable partial order reduction (stubborn sets)\n"
                    "  -a, --siphon-trap <timeout>        Siphon-Trap analysis timeout in seconds (default 0)\n"
                    "      --siphon-depth <place count>   Search depth of siphon (default 0, which counts all places)\n"
                    "  -n, --no-statistics                Do not display any statistics (default is to display it)\n"
                    "  -h, --help                         Display this help message\n"
                    "  -v, --version                      Display version information\n"
                    "  -ctl <type>                        Verify CTL properties\n"
                    "                                     - local     Liu and Smolka's on-the-fly algorithm\n"
                    "                                     - czero     local with certain zero extension (default)\n"
                    "  -c, --cpn-overapproximation        Over approximate query on Colored Petri Nets (CPN only)\n"
                    //"  -g                                 Enable game mode (CTL Only)" // Feature not yet implemented
#ifdef VERIFYPN_MC_Simplification
                    "  -z <number of cores>               Number of cores to use (currently only query simplification)\n"
#endif
                    "  -tar                               Enables Trace Abstraction Refinement for reachability properties\n"
                    "  --write-simplified <filename>      Outputs the queries to the given file after simplification\n"
                    "  --write-reduced <filename>         Outputs the model to the given file after structural reduction\n"
                    "  --binary-query-io <0,1,2,3>        Determines the input/output format of the query-file\n"
                    "                                     - 0 MCC XML format for Input and Output\n"
                    "                                     - 1 Input is binary, output is XML\n"
                    "                                     - 2 Output is binary, input is XML\n"
                    "                                     - 3 Input and Output is binary\n"
                    "\n"
                    "Return Values:\n"
                    "  0   Successful, query satisfiable\n"
                    "  1   Unsuccesful, query not satisfiable\n"
                    "  2   Unknown, algorithm was unable to answer the question\n"
                    "  3   Error, see stderr for error message\n"
                    "\n"
                    "VerifyPN is an untimed CTL verification engine for TAPAAL.\n"
                    "TAPAAL project page: <http://www.tapaal.net>\n");
            return SuccessCode;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERIFYPN_VERSION);
            printf("Copyright (C) 2011-2020\n");
            printf("                        Frederik Meyer Boenneland <sadpantz@gmail.com>\n");
            printf("                        Jakob Dyhr <jakobdyhr@gmail.com>\n");
            printf("                        Peter Fogh <peter.f1992@gmail.com>\n");
            printf("                        Jonas Finnemann Jensen <jopsen@gmail.com>\n");
            printf("                        Lasse Steen Jensen <lassjen88@gmail.com>\n");
            printf("                        Peter Gjøl Jensen <root@petergjoel.dk>\n");
            printf("                        Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>\n");
            printf("                        Mads Johannsen <mads_johannsen@yahoo.com>\n");
            printf("                        Isabella Kaufmann <bellakaufmann93@gmail.com>\n");
            printf("                        Andreas Hairing Klostergaard <kloster92@me.com>\n");
            printf("                        Søren Moss Nielsen <soren_moss@mac.com>\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>\n");
            printf("                        Samuel Pastva <daemontus@gmail.com>\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>\n");
            printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
            return SuccessCode;
        } else if (options.modelfile == NULL) {
            options.modelfile = argv[i];
        } else if (options.queryfile == NULL) {
            options.queryfile = argv[i];
        } else {
			fprintf(stderr, "Argument Error: Unrecognized option \"%s\"\n", options.modelfile);
			return ErrorCode;
        }
    }
    //Print parameters
    if (options.printstatistics) {
        std::cout << std::endl << "Parameters: ";
        for (int i = 1; i < argc; i++) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
    }
    
    if (options.statespaceexploration) {
        // for state-space exploration some options are mandatory
        options.enablereduction = 0;
        options.kbound = 0;
        options.queryReductionTimeout = 0;
        options.lpsolveTimeout = 0;
        options.siphontrapTimeout = 0;
        options.stubbornreduction = false;
//        outputtrace = false;
    }


    //----------------------- Validate Arguments -----------------------//

    //Check for model file
    if (!options.modelfile) {
        fprintf(stderr, "Argument Error: No model-file provided\n");
        return ErrorCode;
    }

    //Check for query file
    if (!options.modelfile && !options.statespaceexploration) {
        fprintf(stderr, "Argument Error: No query-file provided\n");
        return ErrorCode;
    }
    
    return ContinueCode;
}

auto
readQueries(options_t& options, std::vector<std::string>& qstrings)
{

    std::vector<Condition_ptr > conditions;
    if (!options.statespaceexploration) {
        //Open query file
        ifstream qfile(options.queryfile, ifstream::in);
        if (!qfile) {
            fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", options.queryfile);
            fprintf(stdout, "CANNOT_COMPUTE\n");
            conditions.clear();
            return conditions;
        }

        if(options.querynumbers.size() == 0)
        {
            string querystring; // excluding EF and AG

            //Read everything
            stringstream buffer;
            buffer << qfile.rdbuf();
            string querystr = buffer.str(); // including EF and AG
            //Parse XML the queries and querystr let be the index of xmlquery 
        
            qstrings.push_back(querystring);
            //Validate query type
            if (querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG") {
                    fprintf(stderr, "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n", querystr.substr(0, 2).c_str());
                    return conditions;
            }
            //Check if is invariant
            bool isInvariant = querystr.substr(0, 2) == "AG";

            //Wrap in not if isInvariant
            querystring = querystr.substr(2);
            std::vector<std::string> tmp;
            conditions.emplace_back(ParseQuery(querystring, tmp));
            if(isInvariant) conditions.back() = std::make_shared<AGCondition>(conditions.back());
            else            conditions.back() = std::make_shared<EFCondition>(conditions.back());
        }
        else
        {
            std::vector<QueryItem> queries;
            if(options.binary_query_io & 1)
            {
                QueryBinaryParser parser;
                if (!parser.parse(qfile, options.querynumbers)) {
                    fprintf(stderr, "Error: Failed parsing binary query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }     
                queries = std::move(parser.queries);
            }
            else
            {
                QueryXMLParser parser;
                if (!parser.parse(qfile, options.querynumbers)) {
                    fprintf(stderr, "Error: Failed parsing XML query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }
                queries = std::move(parser.queries);
            }

            size_t i = 0;
            for(auto& q : queries)
            {
                if(!options.querynumbers.empty()
                        && options.querynumbers.count(i) == 0)
                {
                    ++i;
                    continue;
                }
                ++i;

                if (q.parsingResult == QueryItem::UNSUPPORTED_QUERY) {
                    fprintf(stdout, "The selected query in the XML query file is not supported\n");
                    fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
                    continue;
                }
                // fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);

                conditions.push_back(q.query);
                if (conditions.back() == nullptr) {
                    fprintf(stderr, "Error: Failed to parse query \"%s\"\n", q.id.c_str()); //querystr.substr(2).c_str());
                    fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
                    conditions.pop_back();
                }
                
                qstrings.push_back(q.id);
            }
        }
        qfile.close();
        return conditions;
    } else { // state-space exploration
        std::string querystring = "false";
        std::vector<std::string> empty;
        conditions.push_back(std::make_shared<EFCondition>(ParseQuery(querystring, empty)));
        return conditions;
    } 
 }

ReturnValue parseModel(AbstractPetriNetBuilder& builder, options_t& options)
{
    //Load the model
    ifstream mfile(options.modelfile, ifstream::in);
    if (!mfile) {
        fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", options.modelfile);
        fprintf(stdout, "CANNOT_COMPUTE\n");
        return ErrorCode;
    }


    //Parse and build the petri net
    PNMLParser parser;
    parser.parse(mfile, &builder);
    options.isCPN = builder.isColored();

    // Close the file
    mfile.close();
    return ContinueCode;
}

void printStats(PetriNetBuilder& builder, options_t& options)
{
    if (options.printstatistics) {
        if (options.enablereduction != 0) {

            std::cout << "Size of net before structural reductions: " <<
                    builder.numberOfPlaces() << " places, " <<
                    builder.numberOfTransitions() << " transitions" << std::endl;             
            std::cout << "Size of net after structural reductions: " <<
                    builder.numberOfPlaces() - builder.RemovedPlaces() << " places, " <<
                    builder.numberOfTransitions() - builder.RemovedTransitions() << " transitions" << std::endl;
            std::cout << "Structural reduction finished after " << builder.getReductionTime() <<
                    " seconds" << std::endl;
            
            std::cout   << "\nNet reduction is enabled.\n";
            builder.printStats(std::cout);
        }
    }
}

void printUnfoldingStats(ColoredPetriNetBuilder& builder, options_t& options) {
    if (options.printstatistics) {
        if (!builder.isColored() && !builder.isUnfolded())
            return;
        std::cout << "\nSize of colored net: " <<
                builder.getPlaceCount() << " places, " <<
                builder.getTransitionCount() << " transitions, and " <<
                builder.getArcCount() << " arcs" << std::endl;
        std::cout << "Size of unfolded net: " <<
                builder.getUnfoldedPlaceCount() << " places, " <<
                builder.getUnfoldedTransitionCount() << " transitions, and " <<
                builder.getUnfoldedArcCount() << " arcs" << std::endl;
        std::cout << "Unfolded in " << builder.getUnfoldTime() << " seconds" << std::endl;
    }
}

std::string getXMLQueries(vector<std::shared_ptr<Condition>> queries, vector<std::string> querynames, std::vector<ResultPrinter::Result> results) {
    bool cont = false;    
    for(uint32_t i = 0; i < results.size(); i++) {
        if (results[i] == ResultPrinter::CTL) {
            cont = true;
            break;
        }
    }
    
    if (!cont) {
        return "";
    }
       
    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    
    for(uint32_t i = 0; i < queries.size(); i++) {
        if (!(results[i] == ResultPrinter::CTL)) {
            continue;
        }
        ss << "  <property>\n    <id>" << querynames[i] << "</id>\n    <description>Simplified</description>\n    <formula>\n";
        queries[i]->toXML(ss, 3);
        ss << "    </formula>\n  </property>\n";
    }
            
    ss << "</property-set>\n";
    
    return ss.str();
}
 
void writeQueries(vector<std::shared_ptr<Condition>>& queries, vector<std::string>& querynames, std::vector<uint32_t>& order, std::string& filename, bool binary, const std::unordered_map<std::string, uint32_t>& place_names) 
{
    fstream out;
    
    if(binary)
    {
        out.open(filename, std::ios::binary | std::ios::out);
        uint32_t cnt = 0;
        for(uint32_t j = 0; j < queries.size(); j++) {
            if(queries[j]->isTriviallyTrue() || queries[j]->isTriviallyFalse()) continue;
            ++cnt;
        }
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        cnt = place_names.size();
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        for(auto& kv : place_names)
        {
            out.write(reinterpret_cast<const char *>(&kv.second), sizeof(uint32_t));
            out.write(kv.first.data(), kv.first.size());
            out.write("\0", sizeof(char));
        }
    }
    else
    {
        out.open(filename, std::ios::out);
        out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    }
    
    for(uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if(queries[i]->isTriviallyTrue() || queries[i]->isTriviallyFalse()) continue;
        if(binary)
        {
            out.write(querynames[i].data(), querynames[i].size());
            out.write("\0", sizeof(char));
            queries[i]->toBinary(out);
        }
        else
        {
            out << "  <property>\n    <id>" << querynames[i] << "</id>\n    <description>Simplified</description>\n    <formula>\n";
            queries[i]->toXML(out, 3);
            out << "    </formula>\n  </property>\n";
        }
    }
        
    if(binary == 0)
    {
        out << "</property-set>\n";
    }
    out.close();
}

int main(int argc, char* argv[]) {

    options_t options;
    
    ReturnValue v = parseOptions(argc, argv, options);
    if(v != ContinueCode) return v;
    options.print();
    srand (time(NULL) xor options.seed_offset);  
    ColoredPetriNetBuilder cpnBuilder;
    if(parseModel(cpnBuilder, options) != ContinueCode) 
    {
        std::cerr << "Error parsing the model" << std::endl;
        return ErrorCode;
    }
    if(options.cpnOverApprox && !cpnBuilder.isColored())
    {
        std::cerr << "CPN OverApproximation is only usable on colored models" << std::endl;
        return UnknownCode;
    }
    if (options.printstatistics) {
        std::cout << "Finished parsing model" << std::endl;
    }

    //----------------------- Parse Query -----------------------//
    std::vector<std::string> querynames;
    auto queries = readQueries(options, querynames);
    
    if(options.printstatistics && options.queryReductionTimeout > 0)
    {
        negstat_t stats;            
        std::cout << "RWSTATS LEGEND:";
        stats.printRules(std::cout);            
        std::cout << std::endl;
    }

    if(cpnBuilder.isColored())
    {
        negstat_t stats;            
        EvaluationContext context(nullptr, nullptr);
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            queries[qid] = queries[qid]->pushNegation(stats, context, false, false, false);
            if(options.printstatistics)
            {
                std::cout << "\nQuery before expansion and reduction: ";
                queries[qid]->toString(std::cout);
                std::cout << std::endl;

                std::cout << "RWSTATS COLORED PRE:";
                stats.print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    if (options.cpnOverApprox) {
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            negstat_t stats;            
            EvaluationContext context(nullptr, nullptr);
            auto q = queries[qid]->pushNegation(stats, context, false, false, false);
            if (!q->isReachability() || q->isLoopSensitive() || stats.negated_fireability) {
                std::cerr << "Warning: CPN OverApproximation is only available for Reachability queries without deadlock, negated fireability and UpperBounds, skipping " << querynames[qid] << std::endl;
                queries.erase(queries.begin() + qid);
                querynames.erase(querynames.begin() + qid);
            }
        }
    }
    
    auto builder = options.cpnOverApprox ? cpnBuilder.stripColors() : cpnBuilder.unfold();
    printUnfoldingStats(cpnBuilder, options);
    builder.sort();
    std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
    ResultPrinter printer(&builder, &options, querynames);
    
    //----------------------- Query Simplification -----------------------//
    bool alldone = options.queryReductionTimeout > 0;
    PetriNetBuilder b2(builder);
    std::unique_ptr<PetriNet> qnet(b2.makePetriNet(false));
    MarkVal* qm0 = qnet->makeInitialMarking();
    ResultPrinter p2(&b2, &options, querynames);

    if(queries.size() == 0 || contextAnalysis(cpnBuilder, b2, qnet.get(), queries) != ContinueCode)
    {
        std::cerr << "Could not analyze the queries" << std::endl;
        return ErrorCode;
    }

    if (options.strategy == PetriEngine::Reachability::OverApprox && options.queryReductionTimeout == 0)
    { 
        // Conflicting flags "-s OverApprox" and "-q 0"
        std::cerr << "Conflicting flags '-s OverApprox' and '-q 0'" << std::endl;
        return ErrorCode;
    }

    // simplification. We always want to do negation-push and initial marking check.
    {
        // simplification. We always want to do negation-push and initial marking check.
        std::vector<LPCache> caches(options.cores);
        std::atomic<uint32_t> to_handle(queries.size());
        auto begin = std::chrono::high_resolution_clock::now();
        auto end = std::chrono::high_resolution_clock::now();
        std::vector<bool> hadTo(queries.size(), true);
        
        do
        {
            auto qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / ( 1 + (to_handle / options.cores));
            if((to_handle <= options.cores || options.cores == 1) && to_handle > 0)
                qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / to_handle;
            std::atomic<uint32_t> cnt(0);
#ifdef VERIFYPN_MC_Simplification

            std::vector<std::thread> threads;
#endif
            std::vector<std::stringstream> tstream(queries.size());
            uint32_t old = to_handle;
            for(size_t c = 0; c < std::min<uint32_t>(options.cores, old); ++c)
            {
#ifdef VERIFYPN_MC_Simplification
                threads.push_back(std::thread([&,c](){ 
#else
                auto simplify = [&,c](){ 
#endif
                    auto& out = tstream[c];
                    auto& cache = caches[c];
                    while(true)
                    {
                    auto i = cnt++;
                    if(i >= queries.size()) return;                
                    if(!hadTo[i]) continue;
                    hadTo[i] = false;
                    negstat_t stats;            
                    EvaluationContext context(qm0, qnet.get());
                    if(options.printstatistics && options.queryReductionTimeout > 0)
                    {
                        out << "\nQuery before reduction: ";
                        queries[i]->toString(out);
                        out << std::endl;
                    }

#ifndef VERIFYPN_MC_Simplification
                    qt = (options.queryReductionTimeout - std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) / (queries.size() - i);              
#endif
                    // this is used later, we already know that this is a plain reachability (or AG)
                    bool wasAGCPNApprox = dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;
                    int preSize=queries[i]->formulaSize(); 
                    queries[i] = Condition::initialMarkingRW([&](){ return queries[i]; }, stats,  context, false, false, true)
                                            ->pushNegation(stats, context, false, false, true);
                    wasAGCPNApprox |= dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;

                    if(options.queryReductionTimeout > 0 && options.printstatistics)
                    {
                        out << "RWSTATS PRE:";
                        stats.print(out);
                        out << std::endl;
                    }

                    if (options.queryReductionTimeout > 0 && qt > 0)
                    {
                        SimplificationContext simplificationContext(qm0, qnet.get(), qt,
                                options.lpsolveTimeout, &cache);
                        try {
                            negstat_t stats;            
                            queries[i] = (queries[i]->simplify(simplificationContext)).formula->pushNegation(stats, context, false, false, true);
                            wasAGCPNApprox |= dynamic_cast<NotCondition*>(queries[i].get()) != nullptr;
                            if(options.printstatistics)
                            {
                                out << "RWSTATS POST:";
                                stats.print(out);
                                out << std::endl;
                            }
                        } catch (std::bad_alloc& ba){
                            std::cerr << "Query reduction failed." << std::endl;
                            std::cerr << "Exception information: " << ba.what() << std::endl;

                            delete[] qm0;
                            std::exit(ErrorCode);
                        }

                        if(options.printstatistics)
                        {
                            out << "\nQuery after reduction: ";
                            queries[i]->toString(out);
                            out << std::endl;
                        }
                        if(simplificationContext.timeout()){
                            if(options.printstatistics)
                                out << "Query reduction reached timeout.\n";
                            hadTo[i] = true;
                        } else {
                            if(options.printstatistics)
                                out << "Query reduction finished after " << simplificationContext.getReductionTime() << " seconds.\n";
                            --to_handle;
                        }
                    }
                    else if(options.printstatistics)
                    {
                        out << "Skipping linear-programming (-q 0)" << std::endl;
                    }
                    if(options.cpnOverApprox && wasAGCPNApprox)
                    {
                        if(queries[i]->isTriviallyTrue())
                            queries[i] = std::make_shared<BooleanCondition>(false);
                        else if(queries[i]->isTriviallyFalse())
                            queries[i] = std::make_shared<BooleanCondition>(true);
                        queries[i]->setInvariant(wasAGCPNApprox);
                    }
                   

                    if(options.printstatistics)
                    {
                        int postSize=queries[i]->formulaSize();
                        double redPerc = preSize-postSize == 0 ? 0 : ((double)(preSize-postSize)/(double)preSize)*100;
                        out << "Query size reduced from " << preSize << " to " << postSize << " nodes ( " << redPerc << " percent reduction).\n";
                    }
                    }
                }
#ifdef VERIFYPN_MC_Simplification
                ));
#else
                ;
                simplify();
#endif
            }
#ifndef VERIFYPN_MC_Simplification
            std::cout << tstream[0].str() << std::endl;
            break;
#else
            for(size_t i = 0; i < std::min<uint32_t>(options.cores, old); ++i)
            {
                threads[i].join();
                std::cout << tstream[i].str();
                std::cout << std::endl;
            }
#endif
            end = std::chrono::high_resolution_clock::now();

        } while(std::any_of(hadTo.begin(), hadTo.end(), [](auto a) { return a;}) && std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < options.queryReductionTimeout && to_handle > 0);
    } 
    
    if(options.query_out_file.size() > 0)
    {
        std::vector<uint32_t> reorder(queries.size());
        for(uint32_t i = 0; i < queries.size(); ++i) reorder[i] = i;
        std::sort(reorder.begin(), reorder.end(), [&queries](auto a, auto b){

            if(queries[a]->isReachability() != queries[b]->isReachability())
                return queries[a]->isReachability() > queries[b]->isReachability();
            if(queries[a]->isLoopSensitive() != queries[b]->isLoopSensitive())
                return queries[a]->isLoopSensitive() < queries[b]->isLoopSensitive();
            if(queries[a]->containsNext() != queries[b]->containsNext())
                return queries[a]->containsNext() < queries[b]->containsNext();
            return queries[a]->formulaSize() < queries[b]->formulaSize();
        });
        writeQueries(queries, querynames, reorder, options.query_out_file, options.binary_query_io & 2, builder.getPlaceNames());
    }
    
    qnet = nullptr;
    delete[] qm0;

    if (!options.statespaceexploration){
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(queries[i]->isTriviallyTrue()){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied).first;
                if(results[i] == ResultPrinter::Ignore && options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (queries[i]->isTriviallyFalse()) {
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::NotSatisfied).first;
                if(results[i] == ResultPrinter::Ignore &&  options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (options.strategy == PetriEngine::Reachability::OverApprox){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Unknown).first;
                if (options.printstatistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
            } else if (!queries[i]->isReachability()) {
                results[i] = ResultPrinter::CTL;
                alldone = false;
            } else {
                queries[i] = queries[i]->prepareForReachability();
                alldone = false;
            }
        }

        if(alldone && options.model_out_file.size() == 0) return SuccessCode;
    }
    
    options.queryReductionTimeout = 0;
    
    //--------------------- Apply Net Reduction ---------------//
        
    if (options.enablereduction > 0) {
        // Compute how many times each place appears in the query
        builder.startTimer();
        builder.reduce(queries, results, options.enablereduction, options.trace, nullptr, options.reductionTimeout, options.reductions);
        printer.setReducer(builder.getReducer());        
    }
    
    printStats(builder, options);
    
    auto net = std::unique_ptr<PetriNet>(builder.makePetriNet());
    
    if(options.model_out_file.size() > 0)
    {
        fstream file;
        file.open(options.model_out_file, std::ios::out);
        net->toXML(file);
    }
    
    if(alldone) return SuccessCode;
    
    //----------------------- Verify CTL queries -----------------------//
    std::vector<size_t> ctl_ids;
    for(size_t i = 0; i < queries.size(); ++i)
    {
        if(results[i] == ResultPrinter::CTL)
        {
            ctl_ids.push_back(i);
        }
    }
    
    if (ctl_ids.size() > 0) {
        options.isctl=true;
        PetriEngine::Reachability::Strategy reachabilityStrategy=options.strategy;

        // Assign indexes
        if(queries.size() == 0 || contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode)
        {
            std::cerr << "An error occurred while assigning indexes" << std::endl;
            return ErrorCode;
        }
        if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::DFS;
        v = CTLMain(net.get(),
            options.ctlalgorithm,
            options.strategy,
            options.gamemode,
            options.printstatistics,
            true,
            options.stubbornreduction,
            querynames,
            queries,
            ctl_ids,
            options);

        if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
            return v;
        }
        // go back to previous strategy if the program continues
        options.strategy=reachabilityStrategy;
    }
    options.isctl=false;
    
    //----------------------- Siphon Trap ------------------------//
    
    if(options.siphontrapTimeout > 0){
        for (uint32_t i = 0; i < results.size(); i ++) {
            bool isDeadlockQuery = std::dynamic_pointer_cast<DeadlockCondition>(queries[i]) != nullptr;
 
            if (results[i] == ResultPrinter::Unknown && isDeadlockQuery) {    
                STSolver stSolver(printer, *net, queries[i].get(), options.siphonDepth);
                stSolver.solve(options.siphontrapTimeout);
                results[i] = stSolver.printResult();
                if (results[i] == Reachability::ResultPrinter::NotSatisfied && options.printstatistics) {
                    std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                }
            }
        }
        
        if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
            return SuccessCode;
        }
    }
    options.siphontrapTimeout = 0;
    
    //----------------------- Reachability -----------------------//

    //Analyse context again to reindex query
    contextAnalysis(cpnBuilder, builder, net.get(), queries);

    // Change default place-holder to default strategy
    if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::HEUR;
    
    if(options.tar && net->numberOfPlaces() > 0)
    {
        //Create reachability search strategy
        TARReachabilitySearch strategy(printer, *net, builder.getReducer(), options.kbound);

        // Change default place-holder to default strategy
        fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
        options.strategy = PetriEngine::Reachability::DFS;

        //Reachability search
        strategy.reachable(queries, results, 
                options.printstatistics,
                options.trace);
    }
    else
    {
        ReachabilitySearch strategy(*net, printer, options.kbound);

        // Change default place-holder to default strategy
        if(options.strategy == DEFAULT) options.strategy = PetriEngine::Reachability::HEUR;

        //Reachability search
        strategy.reachable(queries, results,
                           options.strategy,
                           options.stubbornreduction,
                           options.statespaceexploration,
                           options.printstatistics,
                           options.trace);
    }
       
    return SuccessCode;
}

