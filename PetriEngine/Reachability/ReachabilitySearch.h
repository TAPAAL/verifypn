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
#ifndef BREADTHFIRSTREACHABILITYSEARCH_H
#define BREADTHFIRSTREACHABILITYSEARCH_H

#include <memory>
#include <vector>

#include "../Structures/State.h"
#include "ReachabilityResult.h"
#include "../PQL/PQL.h"
#include "../PetriNet.h"
#include "../Structures/StateSet.h"
#include "../Structures/Queue.h"
#include "../SuccessorGenerator.h"

namespace PetriEngine {
    namespace Reachability {

        enum Strategy {
            BFS,
            DFS,
            HEUR,
            APPROX
        };
        
        /** Implements reachability check in a BFS manner using a hash table */
        class ReachabilitySearch {
        private:
            ResultPrinter& printer;
            

        public:

            ReachabilitySearch(ResultPrinter& printer, PetriNet& net, int kbound = 0)
            : printer(printer), states(net, kbound), _net(net) {
                _kbound = kbound;
            }
            
            ~ReachabilitySearch()
            {
            }

            /** Perform reachability check using BFS with hasing */
            void reachable(                    
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Strategy strategy,
                    bool statespacesearch,
                    bool printstats);
        private:
            struct searchstate_t {
                size_t expandedStates = 0;
                size_t exploredStates = 1;
                std::vector<size_t> enabledTransitionsCount;
                size_t heurquery = 0;               
                bool usequeries;
            };
            
            template<typename Q>
            void tryReach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                bool printstats,
                Q queue);
            void printStats(searchstate_t& s);
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, searchstate_t& );
            ResultPrinter::Result printQuery(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result, searchstate_t& );
            
            int _kbound;
            Structures::StateSet states;
            PetriNet& _net;
        };
        
        template<typename Q>
        void ReachabilitySearch::tryReach(   std::vector<std::shared_ptr<PQL::Condition> >& queries, 
                                        std::vector<ResultPrinter::Result>& results, bool usequeries, 
                                        bool printstats, Q queue)
        {

            // set up state
            searchstate_t ss;
            ss.enabledTransitionsCount.resize(_net.numberOfTransitions(), 0);
            ss.expandedStates = 0;
            ss.exploredStates = 1;
            ss.heurquery = 0;
            ss.usequeries = usequeries;

            // set up working area
            Structures::State state;
            Structures::State working;
            state.setMarking(_net.makeInitialMarking());
            working.setMarking(_net.makeInitialMarking());
            
            // check initial marking
            if(ss.usequeries) 
            {
                if(checkQueries(queries, results, working, ss))
                {
                    if(printstats) printStats(ss);
                        return;
                }
            }
            
            // add initial
            auto r = states.add(state);
            if(r.first) queue.push(r.second, state, queries[ss.heurquery]);

            // Search!
            while (queue.pop(state)) {

                 SuccessorGenerator generator(_net, state);

                while(generator.next(working)){
                    ss.enabledTransitionsCount[generator.fired()]++;
                    auto res = states.add(working);
                    if (res.first) {
                        queue.push(res.second, working, queries[ss.heurquery]);
                        ss.exploredStates++;
                        if (checkQueries(queries, results, working, ss)) {
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

            if(printstats) printStats(ss);
        }

    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
