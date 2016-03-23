/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011-2014  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *			    Jiri Srba <srba.jiri@gmail.com>
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

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/LinearOverApprox.h"
#include "PetriEngine/Reachability/BreadthFirstReachabilitySearch.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"

using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

/** Enumeration of return values from VerifyPN */
enum ReturnValue {
    SuccessCode = 0,
    FailedCode = 1,
    UnknownCode = 2,
    ErrorCode = 3,
    ContinueCode = 4
};

struct options_t {
//    bool outputtrace = false;
    int kbound = 0;
    char* modelfile = NULL;
    char* queryfile = NULL;
    bool disableoverapprox = false;
    int enablereduction = 0; // 0 ... disabled (default),  1 ... aggresive, 2 ... k-boundedness preserving
    bool statespaceexploration = false;
    bool printstatistics = true;
    size_t memorylimit = 2048;
};


#define VERSION  "1.2.0"

ReturnValue contextAnalysis(PetriNetBuilder& builder, Condition* query)
{
    //Context analysis
    AnalysisContext context(builder.getPlaceNames());
    query->analyze(context);

    //Print errors if any
    if (context.errors().size() > 0) {
        for (size_t i = 0; i < context.errors().size(); i++) {
            fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
        }
        return ErrorCode;
    }
    return ContinueCode;
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
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
//            outputtrace = true;
		}else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0){
			if (i==argc-1) {
                                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
				return ErrorCode;                           
                        }
                        ++i;
//                        char* s = argv[++i];
/*			if(strcmp(s, "BestFS") == 0)
				searchstrategy = BestFS;
			else if(strcmp(s, "BFS") == 0)
				searchstrategy = BFS;
			else if(strcmp(s, "DFS") == 0)
				searchstrategy = DFS;
			else if(strcmp(s, "RDFS") == 0)
				searchstrategy = RDFS;
			else if(strcmp(s, "OverApprox") == 0)
				searchstrategy = OverApprox;
			else{
				fprintf(stderr, "Argument Error: Unrecognized search strategy \"%s\"\n", s);
				return ErrorCode;
			}*/
		} else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory-limit") == 0) {
			if (i == argc - 1) {
				fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;
			}
			if (sscanf(argv[++i], "%zu", &options.memorylimit) != 1) {
				fprintf(stderr, "Argument Error: Invalid memory limit \"%s\"\n", argv[i]);
				return ErrorCode;
			}
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable-overapprox") == 0) {
            options.disableoverapprox = true;
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--state-space-exploration") == 0) {
            options.statespaceexploration = true;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-statistics") == 0) {
            options.printstatistics = false;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xml-queries") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            ++i;
/*            if (sscanf(argv[++i], "%zu", &options.xmlquery) != 1 || options.xmlquery <= 0) {
                fprintf(stderr, "Argument Error: Query index to verify \"%s\"\n", argv[i]);
                return ErrorCode;
            }*/
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &options.enablereduction) != 1 || options.enablereduction < 0 || options.enablereduction > 2) {
                fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: verifypn [options] model-file query-file\n"
                    "A tool for answering reachability of place cardinality queries (including deadlock)\n"
                    "for weighted P/T Petri nets extended with inhibitor arcs.\n"
                    "\n"
                    "Options:\n"
                    "  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
                    "  -t, --trace                        Provide XML-trace to stderr\n"
                    "  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
                    "  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
                    "  -d, --disable-over-approximation   Disable linear over approximation\n"
                    "  -r, --reduction                    Enable structural net reduction:\n"
                    "                                     - 0  disabled (default)\n"
                    "                                     - 1  aggressive reduction\n"
                    "                                     - 2  reduction preserving k-boundedness\n"
                    "  -n, --no-statistics                Do not display any statistics (default is to display it)\n"
                    "  -h, --help                         Display this help message\n"
                    "  -v, --version                      Display version information\n"
                    "  -m, --memory-limit <mb-memory>     Limit for when encoding kicks in, default 2048\n"
                    "\n"
                    "Return Values:\n"
                    "  0   Successful, query satisfiable\n"
                    "  1   Unsuccesful, query not satisfiable\n"
                    "  2   Unknown, algorithm was unable to answer the question\n"
                    "  3   Error, see stderr for error message\n"
                    "\n"
                    "VerifyPN is a compilation of PeTe as untimed backend for TAPAAL.\n"
                    "PeTe project page: <https://github.com/jopsen/PeTe>\n"
                    "TAPAAL project page: <http://www.tapaal.net>\n");
            return SuccessCode;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERSION);
            printf("Copyright (C) 2011-2014 Jonas Finnemann Jensen <jopsen@gmail.com>,\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>,\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>,\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
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
    options.memorylimit *= 1024*1024;
    if (options.statespaceexploration) {
        // for state-space exploration some options are mandatory
        options.disableoverapprox = true;
        options.enablereduction = 0;
        options.kbound = 0;
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
readQueries(PNMLParser::TransitionEnablednessMap& tmap, options_t& options)
{
    QueryXMLParser XMLparser(tmap);
    bool isInvariant = false;
    std::vector<std::shared_ptr<Condition > > conditions;
    string querystring; // excluding EF and AG
    if (!options.statespaceexploration) {
        //Open query file
        ifstream qfile(options.queryfile, ifstream::in);
        if (!qfile) {
            fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", options.queryfile);
            fprintf(stdout, "CANNOT_COMPUTE\n");
            conditions.clear();
            return conditions;
        }

        //Read everything
        stringstream buffer;
        buffer << qfile.rdbuf();
        string querystr = buffer.str(); // including EF and AG
        //Parse XML the queries and querystr let be the index of xmlquery 		

        if (!XMLparser.parse(querystr)) {
            fprintf(stderr, "Error: Failed parsing XML query file\n");
            fprintf(stdout, "DO_NOT_COMPETE\n");
            conditions.clear();
            return conditions;
        }
        
        for(auto& q : XMLparser.queries)
        {
            if (q.parsingResult == QueryXMLParser::QueryItem::UNSUPPORTED_QUERY) {
                fprintf(stdout, "The selected query in the XML query file is not supported\n");
                fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q.id.c_str());
                conditions.clear();
                return conditions;
            }
            // fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);
            querystr = q.queryText;
            querystring = querystr.substr(2);
            isInvariant = q.negateResult;

            fprintf(stdout, "FORMULA %s ", q.id.c_str());
            fflush(stdout);
            conditions.push_back(ParseQuery(querystring, isInvariant, q.placeNameForBound));
            if (conditions.back() == NULL) {
                fprintf(stderr, "Error: Failed to parse query \"%s\"\n", querystring.c_str()); //querystr.substr(2).c_str());
                conditions.clear();
                return conditions;
            }
        }
        qfile.close();
        return conditions;
    } else { // state-space exploration
        querystring = "false";
        isInvariant = false;
        conditions.push_back(ParseQuery(querystring, isInvariant, ""));
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

    //Read everything
    stringstream buffer;
    buffer << mfile.rdbuf();

    //Parse and build the petri net
    PNMLParser parser;
    parser.parse(buffer.str(), &builder);
    parser.makePetriNet();

    transitionEnabledness = parser.getTransitionEnabledness(); // Remember conditions for transitions

    // Close the file
    mfile.close();
    return ContinueCode;
}

ReturnValue printResult(Condition* query, ReachabilityResult& result, PetriNetBuilder& builder, options_t& options)
{
    
    ReturnValue retval = ErrorCode;

    if (options.statespaceexploration) {
        retval = UnknownCode;
        unsigned int placeBound = 0;
        for (size_t p = 0; p < result.maxPlaceBound().size(); p++) {
            placeBound = std::max<unsigned int>(placeBound, result.maxPlaceBound()[p]);
        }
        // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
        fprintf(stdout, "STATE_SPACE STATES %lli TECHNIQUES EXPLICIT\n", result.exploredStates());
        fprintf(stdout, "STATE_SPACE TRANSITIONS -1 TECHNIQUES EXPLICIT\n");
        fprintf(stdout, "STATE_SPACE MAX_TOKEN_PER_MARKING %d TECHNIQUES EXPLICIT\n", result.maxTokens());
        fprintf(stdout, "STATE_SPACE MAX_TOKEN_IN_PLACE %d TECHNIQUES EXPLICIT\n", placeBound);
        return retval;
    }

    //Find result code
    if (result.result() == ReachabilityResult::Unknown)
        retval = UnknownCode;
    else if (result.result() == ReachabilityResult::Satisfied)
        retval = query->isInvariant() ? FailedCode : SuccessCode;
    else if (result.result() == ReachabilityResult::NotSatisfied)
        retval = query->isInvariant() ? SuccessCode : FailedCode;

    //Print result
    if (retval == UnknownCode)
        fprintf(stdout, "\nUnable to decide if query is satisfied.\n\n");
    else if (retval == SuccessCode) {
        if(!options.statespaceexploration)
        {
            fprintf(stdout, "TRUE TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION\n");
        }
        fprintf(stdout, "\nQuery is satisfied.\n\n");
    } else if (retval == FailedCode) {
        if (!query->placeNameForBound().empty()) {
            // find index of the place for reporting place bound
            assert(result.maxPlaceBound().size() > 0);
            uint32_t p = builder.getPlaceNames().at(query->placeNameForBound());
            fprintf(stdout, "%d TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION\n", result.maxPlaceBound()[p]);
            fprintf(stdout, "\nMaximum number of tokens in place %s: %d\n\n", 
                    query->placeNameForBound().c_str(), result.maxPlaceBound()[p]);
            retval = UnknownCode;
        } else {
            if(!options.statespaceexploration)
            {
                fprintf(stdout, "FALSE TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION\n");
            }
            fprintf(stdout, "\nQuery is NOT satisfied.\n\n");
        }
    }
    return retval;
}

void printStats(ReachabilityResult& result, PetriNetBuilder& builder, options_t& options)
{
    if (options.printstatistics) {
        //Print statistics
        fprintf(stdout, "STATS:\n");
        fprintf(stdout, "\tdiscovered states: %lli\n", result.discoveredStates());
        fprintf(stdout, "\texplored states:   %lli\n", result.exploredStates());
        fprintf(stdout, "\texpanded states:   %lli\n", result.expandedStates());
        fprintf(stdout, "\tmax tokens:        %i\n", result.maxTokens());
        if (options.enablereduction != 0) {
            fprintf(stdout, "\nNet reduction is enabled.\n");
            fprintf(stdout, "Removed transitions: %zu\n", builder.RemovedTransitions());
            fprintf(stdout, "Removed places: %zu\n", builder.RemovedPlaces());
            fprintf(stdout, "Applications of rule A: %zu\n", builder.RuleA());
            fprintf(stdout, "Applications of rule B: %zu\n", builder.RuleB());
            fprintf(stdout, "Applications of rule C: %zu\n", builder.RuleC());
            fprintf(stdout, "Applications of rule D: %zu\n", builder.RuleD());
        }
        fprintf(stdout, "\nTRANSITION STATISTICS\n");
        for (auto& t : builder.getTransitionNames()) {
            // report how many times transitions were enabled (? means that the transition was removed in net reduction)
            if (t.second == std::numeric_limits<uint32_t>::max()) {
                 std::cout << "<" << t.first << ";?>";
            } else {
                std::cout << "<" << t.first << ";" << result.enabledTransitionsCount()[t.second] << ">";
            }
        }
        fprintf(stdout, "\n\nPLACE-BOUND STATISTICS\n");
        for (auto& p : builder.getPlaceNames())
        {
            // report maximum bounds for each place (? means that the place was removed in net reduction)
            if (p.second == numeric_limits<uint32_t>::max()) {
                std::cout << "<" << p.first << ";?>";
            } else {
                std::cout << "<" << p.first << ";" << result.maxPlaceBound()[p.second] << ">";
            }
        }
        std::cout << std::endl << std::endl;
    }
}

int main(int argc, char* argv[]) {
    options_t options;
    
    ReturnValue v = parseOptions(argc, argv, options);
    if(v != ContinueCode) return v;
    
  
    PetriNetBuilder builder;
    PNMLParser::TransitionEnablednessMap transitionEnabledness;
    
    if(parseModel(transitionEnabledness, builder, options) != ContinueCode) return ErrorCode;

    //----------------------- Parse Query -----------------------//
    auto queries = readQueries(transitionEnabledness, options);
    
    if(queries.size() == 0 || contextAnalysis(builder, queries[0].get()) != ContinueCode)  return ErrorCode;

    std::vector<ReachabilityResult::Result> results(queries.size());
    
    if (!options.disableoverapprox){
        PetriNet* net = builder.makePetriNet();
        MarkVal* m0 = net->makeInitialMarking();  
        LinearOverApprox approx(NULL);
        for(size_t i = 0; i < queries.size(); ++i)
        {
            results[i] = approx.reachable(*net, m0, queries[i].get(), options.memorylimit).result();
        }
        delete net;
        delete[] m0;
    }
    
    //--------------------- Apply Net Reduction ---------------//
        
    if (options.enablereduction == 1 || options.enablereduction == 2) {
        // Compute how many times each place appears in the query
        builder.reduce(queries[0].get(), options.enablereduction);
    }

    //----------------------- Reachability -----------------------//

    //Create reachability search strategy
    ReachabilitySearchStrategy* strategy = new BreadthFirstReachabilitySearch(options.kbound);

    PetriNet* net = builder.makePetriNet();
    MarkVal* m0 = net->makeInitialMarking();  
    
    // analyse context again to reindex query
    contextAnalysis(builder, queries[0].get());
    
    //Reachability search
    ReachabilityResult result = strategy->reachable(*net, m0, queries[0].get(), options.memorylimit);

    ReturnValue retval = printResult(queries[0].get(), result, builder, options);

    printStats(result, builder, options);

    return retval;
}

