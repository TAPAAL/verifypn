/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011-2017  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>,
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 * CTL Extension
 *                          Peter Fogh <pfogh12@student.aau.dk>
 *                          Isabella Kaufmann <ikaufm12@student.aau.dk>
 *                          Tobias Skovgaard Jepsen <tjeps12@student.aau.dk>
 *                          Lasse Steen Jensen <lasjen12@student.aau.dk>
 *                          Søren Moss Nielsen <smni12@student.aau.dk>
 *                          Samuel Pastva <daemontus@gmail.com>
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

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "PetriEngine/errorcodes.h"
#include "PetriEngine/STSolver.h"

#include "CTL/CTLEngine.h"

using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

#define VERSION  "2.1.0"

ReturnValue contextAnalysis(PetriNetBuilder& builder, std::vector<std::shared_ptr<Condition> >& queries)
{
    //Context analysis
    AnalysisContext context(builder.getPlaceNames());
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
                size_t n;
                if(sscanf(qn.c_str(), "%zu", &n) != 1)
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
            if (sscanf(argv[++i], "%d", &options.enablereduction) != 1 || options.enablereduction < 0 || options.enablereduction > 2) {
                fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--partial-order-reduction") == 0) {
            options.stubbornreduction = false;
        } else if(strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--siphon-trap") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.siphontrapTimeout) != 1 || options.siphontrapTimeout < 0) {
                fprintf(stderr, "Argument Error: Invalid siphon-trap timeout \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-ctl") == 0){
            options.isctl = true;
            if(argc > i + 1){
                if(strcmp(argv[i + 1], "local") == 0){
                    i++;
                    options.ctlalgorithm = CTL::Local;
                }
                else if(strcmp(argv[i + 1], "czero") == 0){
                    i++;
                    options.ctlalgorithm = CTL::CZero;
                }

            }
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--game-mode") == 0){
            options.gamemode = true;
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
                    "  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
                    "  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
                    "  -r, --reduction                    Change structural net reduction:\n"
                    "                                     - 0  disabled\n"
                    "                                     - 1  aggressive reduction (default)\n"
                    "                                     - 2  reduction preserving k-boundedness\n"
                    "  -q, --query-reduction <timeout>    Query reduction timeout in seconds (default 30)\n"
                    "                                     write -q 0 to disable query reduction\n"
                    "  -l, --lpsolve-timeout <timeout>    LPSolve timeout in seconds, default 10\n"
                    "  -p, --partial-order-reduction      Disable partial order reduction (stubborn sets)\n"
                    "  -a, --siphon-trap <timeout>        Siphon-Trap analysis timeout in seconds, default 0\n"
                    "  -n, --no-statistics                Do not display any statistics (default is to display it)\n"
                    "  -h, --help                         Display this help message\n"
                    "  -v, --version                      Display version information\n"
                    "  -ctl                               Verify CTL properties\n"
                    "                                     - local     Liu and Smolka's on-the-fly algorithm\n"
                    "                                     - czero     local with certain zero extension (default)\n"
                    //"  -g                                 Enable game mode (CTL Only)" // Feature not yet implemented
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
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERSION);
            printf("Copyright (C) 2011-2017 Jonas Finnemann Jensen <jopsen@gmail.com>,\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>,\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>,\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>,\n");
            printf("                        Peter Gjøl Jensen <root@petergjoel.dk>\n");
            printf("                        Peter Fogh <pfogh12@student.aau.dk>\n");
            printf("                        Isabella Kaufmann <ikaufm12@student.aau.dk>\n");
            printf("                        Tobias Skovgaard Jepsen <tjeps12@student.aau.dk>\n");
            printf("                        Lasse Steen Jensen <lasjen12@student.aau.dk>\n");
            printf("                        Søren Moss Nielsen <smni12@student.aau.dk>\n");
            printf("                        Samuel Pastva <daemontus@gmail.com>\n");
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

    // Check strategy when is CTL (CTLEngine does not support all of them)
    if(options.isctl){
        //Default to DFS (No heuristic strategy)
        if(options.strategy == PetriEngine::Reachability::HEUR){
            options.strategy = PetriEngine::Reachability::DFS;
        }
        else if(options.strategy != PetriEngine::Reachability::DFS && options.strategy != PetriEngine::Reachability::BFS){
            std::cerr << "Argument Error: Invalid CTL search strategy. Valid ones are DFS (default) and BFS." << std::endl;
            return ErrorCode;
        }
    }
    
    // Check if the choosen options are incompatible with upper bound queries
    if(options.stubbornreduction || options.queryReductionTimeout > 0) {
        options.upperboundcheck = true;
    }

    return ContinueCode;
}

auto
readQueries(PNMLParser::TransitionEnablednessMap& tmap, options_t& options, std::vector<std::string>& qstrings)
{

    std::vector<std::shared_ptr<Condition > > conditions;
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
            if (isInvariant)
                    querystring = "not ( " + querystring + " )";
            std::vector<std::string> tmp;
            conditions.push_back(ParseQuery(querystring, isInvariant, tmp));
        }
        else
        {
            QueryXMLParser XMLparser(tmap);
            if (!XMLparser.parse(qfile)) {
                fprintf(stderr, "Error: Failed parsing XML query file\n");
                fprintf(stdout, "DO_NOT_COMPETE\n");
                conditions.clear();
                return conditions;
            }

            size_t i = 0;

            for(auto& q : XMLparser.queries)
            {
                if(!options.querynumbers.empty()
                        && options.querynumbers.count(i) == 0)
                {
                    ++i;
                    continue;
                }
                ++i;

                if (q.parsingResult == QueryXMLParser::QueryItem::UNSUPPORTED_QUERY) {
                    fprintf(stdout, "The selected query in the XML query file is not supported\n");
                    fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
                    continue;
                }
                // fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);

                conditions.push_back(q.query);
                if (conditions.back() == NULL) {
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
        conditions.push_back(ParseQuery(querystring, false, empty));
        return conditions;
    } 
 }

ReturnValue parseModel( PNMLParser::TransitionEnablednessMap& transitionEnabledness,
                        PetriNetBuilder& builder, options_t& options)
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

    transitionEnabledness = parser.getTransitionEnabledness(); // Remember conditions for transitions

    // Close the file
    mfile.close();
    return ContinueCode;
}

void printStats(PetriNetBuilder& builder, options_t& options)
{
    if (options.printstatistics) {
        if (options.enablereduction != 0) {
            std::cout   << "\nNet reduction is enabled.\n"
                        << "Removed transitions: " << builder.RemovedTransitions() << std::endl
                        << "Removed places: " << builder.RemovedPlaces() << std::endl
                        << "Applications of rule A: " << builder.RuleA() << std::endl
                        << "Applications of rule B: " << builder.RuleB() << std::endl
                        << "Applications of rule C: " << builder.RuleC() << std::endl
                        << "Applications of rule D: " << builder.RuleD() << std::endl
                        << "Applications of rule E: " << builder.RuleE() << std::endl;
        }
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
            
    string outputstring = "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    
    for(uint32_t i = 0; i < queries.size(); i++) {
        if (!(results[i] == ResultPrinter::CTL)) {
            continue;
        }
        outputstring += "  <property>\n    <id>" + querynames[i] + "</id>\n    <description>Simplified</description>\n    <formula>\n";
        outputstring += queries[i]->toXML(3);
        outputstring += "    </formula>\n  </property>\n";
    }
            
    outputstring += "</property-set>\n";
    
    return outputstring;
}

int main(int argc, char* argv[]) {
    options_t options;
    
    ReturnValue v = parseOptions(argc, argv, options);
    if(v != ContinueCode) return v;
    
    options.print();
  
    PetriNetBuilder builder;
    PNMLParser::TransitionEnablednessMap transitionEnabledness;
    
    if(parseModel(transitionEnabledness, builder, options) != ContinueCode) return ErrorCode;
    
    //----------------------- CTL Engine - for testing ------------------------//
    if(options.isctl){
        PetriNet* net = builder.makePetriNet();
        ReturnValue rv = CTLMain(net,
                    options.queryfile,
                    options.ctlalgorithm,
                    options.strategy,
                    options.querynumbers,
                    options.gamemode,
                    options.printstatistics,
                    true);
        delete net;
        return rv;
    }
    
    //----------------------- Parse Query -----------------------//
    std::vector<std::string> querynames;
    auto queries = readQueries(transitionEnabledness, options, querynames);
    
    if (options.upperboundcheck) {
        for (uint32_t i = 0; i < queries.size(); i++) {
            if (queries[i]->isUpperBound()) {
                fprintf(stderr, "Error: Invalid options choosen for upper bound query. ");
                fprintf(stderr, "Cannot use stubborn reduction, query simplification, or aggressive structural reduction.\n");
                fprintf(stdout, "CANNOT_COMPUTE\n");
                return ErrorCode;
            }
        }
    }
    
    if(queries.size() == 0 || contextAnalysis(builder, queries) != ContinueCode)  return ErrorCode;
    
    std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
    ResultPrinter printer(&builder, &options, querynames);
    
    //----------------------- Query Simplification -----------------------//
    bool alldone = options.queryReductionTimeout > 0;
    PetriNetBuilder b2(builder);
    PetriNet* qnet = b2.makePetriNet(false);
    MarkVal* qm0 = qnet->makeInitialMarking();
    ResultPrinter p2(&b2, &options, querynames);
    
    if (options.queryReductionTimeout > 0) {
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if (queries[i]->isUpperBound()) continue;
            
            SimplificationContext simplificationContext(qm0, qnet, 
                    options.queryReductionTimeout, options.lpsolveTimeout);
            
            if(options.printstatistics){fprintf(stdout, "\nQuery before reduction: %s\n", queries[i]->toString().c_str());}
            queries[i] = (queries[i]->simplify(SimplificationContext(simplificationContext))).formula;   
            if(options.printstatistics){fprintf(stdout, "Query after reduction:  %s\n", queries[i]->toString().c_str());}
            if(options.printstatistics){
                if(simplificationContext.timeout()){
                    fprintf(stdout, "Query reduction reached timeout.\n");
                } else {
                    fprintf(stdout, "Query reduction finished after %f seconds.\n", simplificationContext.getReductionTime());
                }
            }
            
        }
    }
    
    delete qnet;
    delete[] qm0;
    
    if (!options.statespaceexploration){
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(queries[i]->toString() == "true"){
                results[i] = p2.printResult(i, queries[i].get(), ResultPrinter::Satisfied);
                if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (queries[i]->toString() == "false") {
                results[i] = p2.printResult(i, queries[i].get(), ResultPrinter::NotSatisfied);
                if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (!queries[i]->isReachability()) {
                results[i] = ResultPrinter::CTL;
                alldone = false;
            } else {
                queries[i] = queries[i]->prepareForReachability();
                alldone = false;
            }
        }
        
        if(alldone) return SuccessCode;
    }

    //----------------------- Verify CTL queries -----------------------//
    
    std::string CTLQueries = getXMLQueries(queries, querynames, results);
    
    if (CTLQueries.size() > 0) {
        PetriNet* ctlnet = builder.makePetriNet();
        // Update query indexes
        int ctlQueryCount = std::count(results.begin(), results.end(), ResultPrinter::CTL);
        options.querynumbers.clear();
        
        for (int x = 0; x < ctlQueryCount; x++) {
            options.querynumbers.insert(x);
        }
        
        v = CTLMain(ctlnet,
            options.queryfile,
            options.ctlalgorithm,
            options.strategy,
            options.querynumbers,
            options.gamemode,
            options.printstatistics,
            true,
            CTLQueries);
        
        delete ctlnet;
        
        if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
            return v;
        }
    }

    //--------------------- Apply Net Reduction ---------------//
        
    if (options.enablereduction == 1 || options.enablereduction == 2) {
        // Compute how many times each place appears in the query
        builder.reduce(queries, results, options.enablereduction, options.trace);
        printer.setReducer(builder.getReducer());        
    }
    
    PetriNet* net = builder.makePetriNet();
    
    for(auto& q : queries)
    {
        q->indexPlaces(builder.getPlaceNames());
    }
    
    //----------------------- Siphon Trap ------------------------//
    
    if(options.siphontrapTimeout > 0){
        for (uint32_t i = 0; i < results.size(); i ++) {
            
            bool isDeadlockQuery = queries[i]->toString() == "deadlock";
 
            if (results[i] == ResultPrinter::Unknown && isDeadlockQuery) {    
                STSolver stSolver(printer, *net, queries[i].get());
                stSolver.Solve(options.siphontrapTimeout);
                results[i] = stSolver.PrintResult();
                
                if (results[i] == Reachability::ResultPrinter::NotSatisfied && options.printstatistics) {
                    std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                }
            }
        }
        
        if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
            return 0;
        }
    }
    
    //----------------------- Reachability -----------------------//
    
    //Create reachability search strategy
    ReachabilitySearch strategy(printer, *net, options.kbound);

    //Analyse context again to reindex query
    contextAnalysis(builder, queries);
    
    //Reachability search
    strategy.reachable(queries, results, 
            options.strategy,
            options.stubbornreduction,
            options.statespaceexploration,
            options.printstatistics, 
            options.trace);
    
    printStats(builder, options);
    
    delete net;
    
    return 0;
}

