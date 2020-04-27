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
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/SuccessorGenerator.h"

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
                    EvaluationContext ec(state.marking(), &_net);
                    if(queries[i]->evaluate(ec) == Condition::RTRUE)
                    {
                        auto r = doCallback(queries[i], i, ResultPrinter::Satisfied, ss, states);
                        results[i] = r.first;
                        if(r.second)
                            return true;
                    }
                    else
                    {
                        alldone = false;
                    }
                }
                if( i == ss.heurquery &&
                    results[i] != ResultPrinter::Unknown)
                {
                    if(queries.size() >= 2)
                    {
                        for(size_t n = 1; n < queries.size(); ++n)
                        {
                            ss.heurquery = (ss.heurquery + n) % queries.size();
                            if(results[i] == ResultPrinter::Unknown)
                                break;
                        }                        
                    }
                }
            }  
            return alldone;
        }        
        
        std::pair<ResultPrinter::Result,bool> ReachabilitySearch::doCallback(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result r,
                                                             searchstate_t& ss, Structures::StateSetInterface* states)
        {
            return _callback.handle(i, query.get(), r, &states->maxPlaceBound(),
                        ss.expandedStates, ss.exploredStates, states->discovered(), states->maxTokens(),
                        states, _satisfyingMarking, _initial.marking());
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
        
#define TRYREACHPAR    (queries, results, usequeries, printstats)
#define TEMPPAR(X, Y)  if(keep_trace) return tryReach<X, Structures::TracableStateSet, Y>TRYREACHPAR ; \
                       else return tryReach<X, Structures::StateSet, Y> TRYREACHPAR;
#define TRYREACH(X)    if(stubbornreduction) TEMPPAR(X, ReducingSuccessorGenerator) \
                       else TEMPPAR(X, SuccessorGenerator)
        

        bool ReachabilitySearch::reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool stubbornreduction,
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
                    std::cerr << "UNSUPPORTED SEARCH STRATEGY" << std::endl;
                    exit(ErrorCode);
            }
        }
    }
}
