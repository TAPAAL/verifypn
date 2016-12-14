/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#include "ReachabilitySearch.h"
#include "../PQL/PQL.h"
#include "../PQL/Contexts.h"
#include "../Structures/StateSet.h"
#include "PetriEngine/SuccessorGenerator.h"

#include <list>
#include <string.h>
#include <set>

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine {
    namespace Reachability {
        
        bool ReachabilitySearch::checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                                std::vector<ResultPrinter::Result>& results,
                                                State& state,
                                                searchstate_t& ss, StateSetInterface* states)
        {
            if(!ss.usequeries) return false;
            
            bool alldone = true;
            for(size_t i = 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    if(queries[i]->evaluate(state, &_net))
                    {
                        results[i] = printQuery(queries[i], i, ResultPrinter::Satisfied, ss, states);                    
                    }
                    else
                    {
                        alldone = false;
                    }
                }
                if( i == ss.heurquery &&
                    results[i] != ResultPrinter::Unknown) ++ss.heurquery;
            }  
            return alldone;
        }        
        
        ResultPrinter::Result ReachabilitySearch::printQuery(std::shared_ptr<PQL::Condition>& query, size_t i,  ResultPrinter::Result r,
                                                                searchstate_t& ss, Structures::StateSetInterface* states)
        {
            return printer.printResult(i, query.get(), r,
                            ss.expandedStates, ss.exploredStates, states->discovered(),
                            ss.enabledTransitionsCount, states->maxTokens(), 
                            states->maxPlaceBound(), states, _satisfyingMarking);  
        }
        
        void ReachabilitySearch::printStats(searchstate_t& ss, Structures::StateSetInterface* states)
        {
            std::cout   << "STATS:\n"
                        << "\tdiscovered states: " << states->discovered() << std::endl
                        << "\texplored states:   " << ss.exploredStates << std::endl
                        << "\texpanded states:   " << ss.expandedStates << std::endl
                        << "\tmax tokens:        " << states->maxTokens() << std::endl;
            
            
            std::cout << "\nTRANSITION STATISTICS\n";
            for (size_t i = 0; i < _net.numberOfTransitions(); ++i) {
                std::cout << "<" << _net.transitionNames()[i] << ":" 
                        << ss.enabledTransitionsCount[i] << ">";                
            }
            // report how many times transitions were enabled (? means that the transition was removed in net reduction)
            for(size_t i = _net.numberOfTransitions(); i < _net.transitionNames().size(); ++i)
            {
                std::cout << "<" << _net.transitionNames()[i] << ":?>";                
            }
            
            
            std::cout << "\n\nPLACE-BOUND STATISTICS\n";
            for (size_t i = 0; i < _net.numberOfPlaces(); ++i) 
            {
                std::cout << "<" << _net.placeNames()[i] << ";" << states->maxPlaceBound()[i] << ">";
            }
            
            // report maximum bounds for each place (? means that the place was removed in net reduction)
            for(size_t i = _net.numberOfPlaces(); i < _net.placeNames().size(); ++i)
            {
                std::cout << "<" << _net.placeNames()[i] << ";?>";                
            }
            
            std::cout << std::endl << std::endl;
        }
        
#define TRYREACHPAR (queries, results, usequeries, printstats)
#define TRYREACH(X) if(keep_trace) tryReach<X, Structures::TracableStateSet>TRYREACHPAR ; \
                    else tryReach<X, Structures::StateSet> TRYREACHPAR;
        
        
        void ReachabilitySearch::reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool statespacesearch,
                    bool printstats,
                    bool keep_trace)
        {
            bool usequeries = !statespacesearch;

            // if we are searching for bounds
            if(!usequeries) strategy = BFS;
            
            switch(strategy)
            {
                case DFS:    
                    TRYREACH(DFSQueue)                        
                    break;
                case BFS:
                    TRYREACH(BFSQueue)
                    break;
                case HEUR:
                    TRYREACH(HeuristicQueue)
                    break;
                case RDFS:
                    TRYREACH(RDFSQueue)
                    break;
                default:
                    std::cout << "UNSUPPORTED SEARCH STRATEGY" << std::endl;
            }
        }
    }
}
