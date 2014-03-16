/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *					   Jiri Srba <srba.jiri@gmail.com>
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
#include <PetriParse/PNMLParser.h>

#include <stdio.h>
#include <PetriEngine/PetriNetBuilder.h>
#include <PetriEngine/PQL/PQL.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <PetriEngine/PQL/PQLParser.h>
#include <PetriEngine/PQL/Contexts.h>

#include <PetriEngine/Reachability/LinearOverApprox.h>
#include <PetriEngine/Reachability/UltimateSearch.h>
#include <PetriEngine/Reachability/RandomDFS.h>
#include <PetriEngine/Reachability/DepthFirstReachabilitySearch.h>
#include <PetriEngine/Reachability/BreadthFirstReachabilitySearch.h>

#include "PetriEngine/Reducer.h"


using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

/** Enumeration of return values from VerifyPN */
enum ReturnValues{
	SuccessCode	= 0,
	FailedCode	= 1,
	UnknownCode	= 2,
	ErrorCode	= 3
};

/** Enumeration of search-strategies in VerifyPN */
enum SearchStrategies{
	BestFS,			//LinearOverAprox + UltimateSearch
	BFS,			//LinearOverAprox + BreadthFirstReachabilitySearch
	DFS,			//LinearOverAprox + DepthFirstReachabilitySearch
	RDFS,			//LinearOverAprox + RandomDFS
	OverApprox		//LinearOverApprx
};

#define VERSION		"1.1.0"

int main(int argc, char* argv[]){
	// Commandline arguments
	bool outputtrace = false;
	int kbound = 0;
	SearchStrategies searchstrategy = BestFS;
	int memorylimit = 3*1024*1024*1024;
	char* modelfile = NULL;
	char* queryfile = NULL;
	bool disableoverapprox = false;
        int enablereduction = 0; // 0 ... disabled (default),  1 ... aggresive, 2 ... k-boundedness preserving

	//----------------------- Parse Arguments -----------------------//

	// Parse command line arguments
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--k-bound") == 0){
                        if (i==argc-1) {
                                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
				return ErrorCode;                           
                        }
                        if(sscanf(argv[++i], "%d", &kbound) != 1 || kbound < 0){
				fprintf(stderr, "Argument Error: Invalid number of tokens \"%s\"\n", argv[i]);
				return ErrorCode;
			}
		}else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
			outputtrace = true;
		}else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0){
			if (i==argc-1) {
                                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
				return ErrorCode;                           
                        }
                        char* s = argv[++i];
			if(strcmp(s, "BestFS") == 0)
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
			}
		}else if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory-limit") == 0){
                        if (i==argc-1) {
                                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;                           
                        }
			if(sscanf(argv[++i], "%d", &memorylimit) != 1 || memorylimit < 0){
				fprintf(stderr, "Argument Error: Invalid memory limit \"%s\"\n", argv[i]);
				return ErrorCode;
			}else
				memorylimit *= 1024;
		}else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable-overapprox") == 0){
			disableoverapprox = true;
                }else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0){
                        if (i==argc-1) {
                                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;                           
                        }
                        if(sscanf(argv[++i], "%d", &enablereduction) != 1 || enablereduction < 0 || enablereduction > 2){
				fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
				return ErrorCode;
			}
		}else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			printf(	"Usage: verifypn [options] model-file query-file\n"
					"A tool for answering reachability of place cardinality queries (including deadlock)\n" 
                                        "for weighted P/T Petri nets extended with inhibitor arcs.\n"
					"\n"
					"Options:\n"
					"  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
					"  -t, --trace                        Provide XML-trace to stderr\n"
					"  -s, --search-strategy <strategy>   Search strategy:\n"
					"                                     - BestFS       Heuristic search (default)\n"
					"                                     - BFS          Breadth first search\n"
					"                                     - DFS          Depth first search\n"
					"                                     - RDFS         Random depth first search\n"
					"                                     - OverApprox   Linear over-approximation only\n"
					"  -m, --memory-limit <megabyte>      Memory limit for state space in MB,\n"
					"                                     0 for unlimited (3 GB default)\n"
					"  -d, --disable-over-approximation   Disable linear over approximation\n"
                                        "  -r, --reduction                    Enable structural net reduction:\n"
                                        "                                     - 0  disabled (default)\n"
                                        "                                     - 1  aggressive reduction\n"
                                        "                                     - 2  reduction preserving k-boundedness\n"
					"  -h, --help                         Display this help message\n"
					"  -v, --version                      Display version information\n"
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
			return 0;
		}else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0){
			printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERSION);
			printf("Copyright (C) 2011-2014 Jonas Finnemann Jensen <jopsen@gmail.com>,\n");
			printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>,\n");
			printf("                        Lars Kærlund Østergaard <larsko@gmail.com>\n");
			printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
                        printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
			return 0;
		}else if(modelfile == NULL){
			modelfile = argv[i];
		}else if(queryfile == NULL){
			queryfile = argv[i];
		}else{
			fprintf(stderr, "Argument Error: Unrecognized option \"%s\"\n", modelfile);
			return ErrorCode;
		}
	}

	//----------------------- Validate Arguments -----------------------//

	//Check for model file
	if(!modelfile){
		fprintf(stderr, "Argument Error: No model-file provided\n");
		return ErrorCode;
	}

	//Check for query file
	if(!modelfile){
		fprintf(stderr, "Argument Error: No query-file provided\n");
		return ErrorCode;
	}

	//----------------------- Open Model -----------------------//

	//Load the model, begin scope to release memory from the stack
	PetriNet* net = NULL;
	MarkVal* m0 = NULL;
	VarVal* v0 = NULL;
                  
        // List of inhibitor arcs
        PNMLParser::InhibitorArcList inhibarcs;
        
	{
		//Load the model
		ifstream mfile(modelfile, ifstream::in);
		if(!mfile){
			fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", modelfile);
			return ErrorCode;
		}

		//Read everything
		stringstream buffer;
		buffer << mfile.rdbuf();

		//Parse and build the petri net
		PetriNetBuilder builder(false);
		PNMLParser parser;
		parser.parse(buffer.str(), &builder);
		parser.makePetriNet();
                
                inhibarcs = parser.getInhibitorArcs(); // Remember inhibitor arcs

		//Build the petri net
		net = builder.makePetriNet();
		m0 = builder.makeInitialMarking();
		v0 = builder.makeInitialAssignment();

		// Close the file
		mfile.close();
	}

	//----------------------- Parse Query -----------------------//

	//Condition to check
	Condition* query = NULL;
	bool isInvariant = false;

	//Read query file, begin scope to release memory
	{
		//Open query file
		ifstream qfile(queryfile, ifstream::in);
		if(!qfile){
			fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", queryfile);
			return ErrorCode;
		}

		//Read everything
		stringstream buffer;
		buffer << qfile.rdbuf();
		string querystr = buffer.str();

		//Validate query type
		if(querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG"){
			fprintf(stderr, "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n", querystr.substr(0, 2).c_str());
			return ErrorCode;
		}

		//Check if is invariant
		isInvariant = querystr.substr(0, 2) == "AG";

		//Warp in not if isInvariant
		string querystring = querystr.substr(2);
		if(isInvariant)
			querystring = "not ( " + querystring + " )";

		//Parse query
		query = ParseQuery(querystring);
		if(!query){
			fprintf(stderr, "Error: Failed to parse query \"%s\"\n", querystr.substr(2).c_str());
			return ErrorCode;
		}

		//Close query file
		qfile.close();
	}

	//----------------------- Context Analysis -----------------------//

	//Create scope for AnalysisContext
	{
		//Context analysis
		AnalysisContext context(*net);
		query->analyze(context);

		//Print errors if any
		if(context.errors().size() > 0){
			for(size_t i = 0; i < context.errors().size(); i++){
				fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
			}
			return ErrorCode;
		}
	}

        //--------------------- Apply Net Reduction ---------------//
        
    Reducer reducer = Reducer(net); // reduced is needed also in trace generation (hence the extended scope)
	if (enablereduction == 1 or enablereduction == 2) {
		// Compute how many times each place appears in the query
		MarkVal* placeInQuery = new MarkVal[net->numberOfPlaces()];
		for (size_t i = 0; i < net->numberOfPlaces(); i++) {
			placeInQuery[i] = 0;
		}
		QueryPlaceAnalysisContext placecontext(*net, placeInQuery);
		query->analyze(placecontext);

		// Compute the places and transitions that connect to inhibitor arcs
		MarkVal* placeInInhib = new MarkVal[net->numberOfPlaces()];
		MarkVal* transitionInInhib = new MarkVal[net->numberOfTransitions()];

		// CreateInhibitorPlacesAndTransitions translates inhibitor place/transitions names to indexes
		reducer.CreateInhibitorPlacesAndTransitions(net, inhibarcs, placeInInhib, transitionInInhib);

		//reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib); 
		reducer.Reduce(net, m0, placeInQuery, placeInInhib, transitionInInhib, enablereduction); // reduce the net
		//reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib);
	}
        
	//----------------------- Reachability -----------------------//

	//Create reachability search strategy
	ReachabilitySearchStrategy* strategy = NULL;
	if(searchstrategy == BestFS)
		strategy = new UltimateSearch(true, kbound, memorylimit);
	else if(searchstrategy == BFS)
		strategy = new BreadthFirstReachabilitySearch(kbound, memorylimit);
	else if(searchstrategy == DFS)
		strategy = new DepthFirstReachabilitySearch(kbound, memorylimit);
	else if(searchstrategy == RDFS)
		strategy = new RandomDFS(kbound, memorylimit);
	else if(searchstrategy == OverApprox)
		strategy = NULL;
	else{
		fprintf(stderr, "Error: Search strategy selection out of range.\n");
		return ErrorCode;
	}

	// Wrap in linear over-approximation, if not disabled
	if(!disableoverapprox)
		strategy = new LinearOverApprox(strategy);

	// If no strategy is provided
	if(!strategy){
		fprintf(stderr, "Error: No search strategy provided!\n");
		return ErrorCode;
	}

	//Reachability search
	ReachabilityResult result = strategy->reachable(*net, m0, v0, query);

	//----------------------- Output Trace -----------------------//
        const std::vector<unsigned int>& trace = (enablereduction==0 ? result.trace() : reducer.NonreducedTrace(net,result.trace()));
	const std::vector<std::string>& tnames = net->transitionNames();
	const std::vector<std::string>& pnames = net->placeNames();

	//Print result to stderr
	if(outputtrace && result.result() == ReachabilityResult::Satisfied){
		fprintf(stderr, "Trace:\n<trace>\n");
		for(size_t i = 0; i < trace.size(); i++){
			fprintf(stderr, "\t<transition id=\"%s\">\n", tnames[trace[i]].c_str());
			for(unsigned int p = 0; p < net->numberOfPlaces(); p++){
				if(net->inArc(p, trace[i])) {
					for (int weight=1; weight<= net->inArc(p, trace[i]); weight++) {
       		                              fprintf(stderr, "\t\t<token place=\"%s\" age=\"0\"/>\n", pnames[p].c_str());
                                    }
  				}
			}
			fprintf(stderr, "\t</transition>\n");
		}
		fprintf(stderr, "</trace>\n");
	}

	//----------------------- Output Statistics -----------------------//

	//Print statistics
	fprintf(stdout, "STATS:\n");
	fprintf(stdout, "\tdiscovered states: %lli\n", result.discoveredStates());
	fprintf(stdout, "\texplored states:   %lli\n", result.exploredStates());
	fprintf(stdout, "\texpanded states:   %lli\n", result.expandedStates());
	fprintf(stdout, "\tmax tokens:        %i\n", result.maxTokens());
        if (enablereduction!=0) {
                fprintf(stdout, "\nNet reduction is enabled.\n");
                fprintf(stdout, "Removed transitions: %d\n", reducer.RemovedTransitions());
                fprintf(stdout, "Removed places: %d\n", reducer.RemovedPlaces());
                fprintf(stdout, "Applications of rule A: %d\n", reducer.RuleA());
                fprintf(stdout, "Applications of rule B: %d\n", reducer.RuleB());
                fprintf(stdout, "Applications of rule C: %d\n", reducer.RuleC());
                fprintf(stdout, "Applications of rule D: %d\n", reducer.RuleD()); 
        }
	fprintf(stdout,"\nTRANSITION STATISTICS\n");
	for(size_t t = 0; t < result.enabledTransitionsCount().size(); t++) { 
		// report how many times transitions were enabled (? means that the transition was removed in net reduction)
		if (net->isTransitionSkipped(t)) {
			fprintf(stdout,"<%s:?> ", tnames[t].c_str());
		} else {
			fprintf(stdout,"<%s:%lli> ", tnames[t].c_str(), result.enabledTransitionsCount()[t]);	
		}
	}
	fprintf(stdout,"\n\nPLACE-BOUND STATISTICS\n");
	for(size_t p = 0; p < result.maxPlaceBound().size(); p++) { 
		// report maximum bounds for each place (? means that the place was removed in net reduction)
		//if (net->isTransitionSkipped(t)) {
		//	fprintf(stdout,"<%s:?> ", tnames[t].c_str());
		//} else {
			fprintf(stdout,"<%s;%i> ", pnames[p].c_str(), result.maxPlaceBound()[p]);	
		//}
	}
	fprintf(stdout,"\n\n");
	
	//----------------------- Output Result -----------------------//

	ReturnValues retval = ErrorCode;

	//Find result code
	if(result.result() == ReachabilityResult::Unknown)
		retval = UnknownCode;
	else if(result.result() == ReachabilityResult::Satisfied)
		retval = isInvariant ? FailedCode : SuccessCode;
	else if(result.result() == ReachabilityResult::NotSatisfied)
		retval = isInvariant ? SuccessCode : FailedCode;

	//Print result
	if(retval == UnknownCode)
		fprintf(stdout, "Unable to decide if query is satisfied.\n");
	else if(retval == SuccessCode)
		fprintf(stdout, "Query is satisfied.\n");
	else if(retval == FailedCode)
		fprintf(stdout, "Query is NOT satisfied.\n");

	return retval;
}

