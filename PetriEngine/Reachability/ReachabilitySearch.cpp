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

#include <list>
#include <string.h>

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine {
    namespace Reachability {

        void ReachabilitySearch::tryReach(
            PetriNet &net,
            const MarkVal *m0,
            std::vector<std::shared_ptr<PQL::Condition > >& queries,
            size_t memorylimit,
            std::vector<ResultPrinter::Result>& results)
        {
                        //Create StateSet
            states = new StateSet(net, _kbound, memorylimit);

            State* state = new State();
            State* working = new State();
            state->setMarking(net.makeInitialMarking());
            working->setMarking(net.makeInitialMarking());
            
            enabledTransitionsCount.resize(net.numberOfTransitions(), 0);

            bool usequeries = false;
            {
                bool alldone = true;
                for(size_t i = 0; i < queries.size(); ++i)
                {
                    if(results[i] == ResultPrinter::Unknown)
                    {
                        usequeries |= queries[i]->placeNameForBound().empty();
                        if(queries[i]->evaluate(*state, &net))
                        {
                            results[i] = printer.printResult(i, queries[i].get(), ResultPrinter::Satisfied, "Query was satisfied in the initial marking",
                                expandedStates, exploredStates, states->discovered(), enabledTransitionsCount, states->maxTokens(), states->maxPlaceBound());
                        }
                        else
                        {
                            alldone = false;
                        }
                    }
                    if( i == _heurquery &&
                        results[i] != ResultPrinter::Unknown) ++_heurquery;
                }
                if(alldone) return;
            }
            
            if(!usequeries) _strategy = BFS;

            auto r = states->add(state);
            if(r.first) pushOnQueue(r.second, state, queries[_heurquery].get(), &net);
            
            while (nextWaiting(state)) {

                net.reset(state);

                while(net.next(working)){
                    enabledTransitionsCount[net.fireing()]++;
                    auto res = states->add(working);
                    if (res.first) {
                        pushOnQueue(res.second, working, queries[_heurquery].get(), &net);
                        exploredStates++;
                        if (usequeries) {
                            bool alldone = true;
                            for(size_t i = 0; i < queries.size(); ++i)
                            {
                                if(results[i] == ResultPrinter::Unknown)
                                {
                                    if(queries[i]->evaluate(*working, &net))
                                    {
                                        results[i] = printer.printResult(i, queries[i].get(), ResultPrinter::Satisfied, "A state satisfying the query was found.",
                                            expandedStates, exploredStates, states->discovered(), enabledTransitionsCount, states->maxTokens(), states->maxPlaceBound());                    
                                    }
                                    else
                                    {
                                        alldone = false;
                                    }
                                }
                                if( i == _heurquery &&
                                    results[i] != ResultPrinter::Unknown) ++_heurquery;
                            }  
                            if(alldone) return;
                        }
                    }
                }
                expandedStates++;
            }
            
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = printer.printResult(i, queries[i].get(), ResultPrinter::NotSatisfied, "No state satisfying the query exists.",
                        expandedStates, exploredStates, states->discovered(), enabledTransitionsCount, states->maxTokens(), states->maxPlaceBound());                    
                }
            }            
        }
        
        void ReachabilitySearch::printStats(PetriNet &net)
        {
            std::cout   << "STATS:\n"
                        << "\tdiscovered states: " << states->discovered() << std::endl
                        << "\texplored states:   " << exploredStates << std::endl
                        << "\texpanded states:   " << expandedStates << std::endl
                        << "\tmax tokens:        " << states->maxTokens() << std::endl;
            
            
            std::cout << "\nTRANSITION STATISTICS\n";
            for (size_t i = 0; i < net.transitionNames().size(); ++i) {
                // report how many times transitions were enabled (? means that the transition was removed in net reduction)
                std::cout << "<" << net.transitionNames()[i] << ";" 
                        << enabledTransitionsCount[i] << ">";
                
            }
            std::cout << "\n\nPLACE-BOUND STATISTICS\n";
            for (size_t i = 0; i < net.placeNames().size(); ++i) 
            {
                // report maximum bounds for each place (? means that the place was removed in net reduction)
                std::cout << "<" << net.placeNames()[i] << ";" << states->maxPlaceBound()[i] << ">";
            }
            std::cout << std::endl << std::endl;
        }
        
        bool ReachabilitySearch::nextWaiting(Structures::State* state)
        {
            switch(_strategy)
            {
                case DFS:
                case HEUR:
                {
                    if(_queue.empty()) return false;
                    auto it = _queue.top();
                    _queue.pop();
                    states->decode(state, it.item);
                    return true;
                }           
                default:
                    return states->nextWaiting(state);
            }
        }
        
        void ReachabilitySearch::pushOnQueue(size_t index, Structures::State* state, PQL::Condition* query,
                PetriNet* net)
        {
            switch(_strategy)
            {
                case DFS:
                    _queue.emplace(index, index);
                    return;
                case HEUR:
                    {
                        DistanceContext context(net, state->marking());
                        // invert result, highest numbers are on top!
                        size_t dist = std::numeric_limits<size_t>::max() - query->distance(context);
                        _queue.emplace(dist, index);
                        return;
                    }
                default:
                    ; // nothing
            }
        }
        
        void ReachabilitySearch::reachable(
                    PetriNet &net,
                    const MarkVal *m0,
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    size_t memorylimit,
                    std::vector<ResultPrinter::Result>& results,
                    bool printstats)
        {

            tryReach(net, m0, queries, memorylimit, results);

            if(printstats) printStats(net);
        }
    }
}
