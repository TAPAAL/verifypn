/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine {
    namespace Reachability {

        void ReachabilitySearch::tryReach(
            std::vector<std::shared_ptr<PQL::Condition > >& queries,
            std::vector<ResultPrinter::Result>& results,
            Strategy strategy,
            bool statespacesearch,
            bool printstats)
        {

            // set up state
            searchstate_t ss;
            ss.enabledTransitionsCount.resize(_net.numberOfTransitions(), 0);
            ss.expandedStates = 0;
            ss.exploredStates = 1;
            ss.heurquery = 0;
            ss.strategy = strategy;
            ss.usequeries = !statespacesearch;
            
            // set up working area
            State state;
            State working;
            state.setMarking(_net.makeInitialMarking());
            working.setMarking(_net.makeInitialMarking());
            SuccessorGenerator generator(_net);
            
            // check initial marking
            if(statespacesearch) checkQueries(queries, results, working, ss);
            
            // if we are searching for bounds
            if(!ss.usequeries) ss.strategy = BFS;

            // add initial
            auto r = states.add(&state);
            if(r.first) pushOnQueue(r.second, state, queries[ss.heurquery], ss);
            
            // Search!
            while (nextWaiting(state, ss)) {

                generator.reset(&state);

                while(generator.next(&working)){
                    ss.enabledTransitionsCount[generator.fired()]++;
                    auto res = states.add(&working);
                    if (res.first) {
                        pushOnQueue(res.second, working, queries[ss.heurquery], ss);
                        ss.exploredStates++;
                        if (checkQueries(queries, results, working, ss)) {
                            delete[] state.marking();
                            delete[] working.marking();         
                            if(printstats) printStats(ss);
                            return;
                        }
                    }
                }
                ss.expandedStates++;
            }
            
            // no more successors, print last results
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = printQuery(queries[i], i, ResultPrinter::NotSatisfied, ss);                    
                }
            }            
            
            delete[] state.marking();
            delete[] working.marking();
            
            if(printstats) printStats(ss);
        }
        
        bool ReachabilitySearch::checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                                std::vector<ResultPrinter::Result>& results,
                                                State& state,
                                                searchstate_t& ss)
        {
            if(!ss.usequeries) return false;
            
            bool alldone = true;
            for(size_t i = 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    ss.usequeries &= !queries[i]->placeNameForBound().empty();
                    if(queries[i]->evaluate(state, &_net))
                    {
                        results[i] = printQuery(queries[i], i, ResultPrinter::Satisfied, ss);                    
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
                                                                searchstate_t& ss)
        {
            return printer.printResult(i, query.get(), r,
                            ss.expandedStates, ss.exploredStates, states.discovered(),
                            ss.enabledTransitionsCount, states.maxTokens(), 
                            states.maxPlaceBound());  
        }
        
        void ReachabilitySearch::printStats(searchstate_t& ss)
        {
            std::cout   << "STATS:\n"
                        << "\tdiscovered states: " << states.discovered() << std::endl
                        << "\texplored states:   " << ss.exploredStates << std::endl
                        << "\texpanded states:   " << ss.expandedStates << std::endl
                        << "\tmax tokens:        " << states.maxTokens() << std::endl;
            
            
            std::cout << "\nTRANSITION STATISTICS\n";
            for (size_t i = 0; i < _net.transitionNames().size(); ++i) {
                // report how many times transitions were enabled (? means that the transition was removed in net reduction)
                std::cout << "<" << _net.transitionNames()[i] << ";" 
                        << ss.enabledTransitionsCount[i] << ">";
                
            }
            std::cout << "\n\nPLACE-BOUND STATISTICS\n";
            for (size_t i = 0; i < _net.placeNames().size(); ++i) 
            {
                // report maximum bounds for each place (? means that the place was removed in net reduction)
                std::cout << "<" << _net.placeNames()[i] << ";" << states.maxPlaceBound()[i] << ">";
            }
            std::cout << std::endl << std::endl;
        }
        
        bool ReachabilitySearch::nextWaiting(Structures::State& state, searchstate_t& ss)
        {
            switch(ss.strategy)
            {
                case DFS:
                case HEUR:
                {
                    if(ss.queue.empty()) return false;
                    auto it = ss.queue.top();
                    ss.queue.pop();
                    states.decode(&state, it.item);
                    return true;
                }           
                default:
                    return states.nextWaiting(&state);
            }
        }
        
        void ReachabilitySearch::pushOnQueue(size_t index, Structures::State& state, std::shared_ptr<PQL::Condition>& query, searchstate_t& ss)
        {
            switch(ss.strategy)
            {
                case DFS:
                    ss.queue.emplace(index, index);
                    return;
                case HEUR:
                    {
                        DistanceContext context(&_net, state.marking());
                        // invert result, highest numbers are on top!
                        uint32_t dist = std::numeric_limits<uint32_t>::max() - query->distance(context);
                        ss.queue.emplace(dist, index);
                        return;
                    }
                default:
                    ; // nothing
            }
        }
        
        void ReachabilitySearch::reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool statespacesearch,
                    bool printstats)
        {

            tryReach(queries, results, strategy, statespacesearch, printstats);
        }
    }
}
