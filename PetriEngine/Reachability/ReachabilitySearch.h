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
#include "../ReducingSuccessorGenerator.h"

namespace PetriEngine {
    namespace Reachability {

        enum Strategy {
            BFS,
            DFS,
            HEUR,
            RDFS,
            OverApprox,
            DEFAULT
        };
        
        /** Implements reachability check in a BFS manner using a hash table */
        class ReachabilitySearch {
        private:
            ResultPrinter& printer;
            

        public:

            ReachabilitySearch(ResultPrinter& printer, PetriNet& net, int kbound = 0)
            : printer(printer), _net(net) {
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
                    bool usestubborn,
                    bool statespacesearch,
                    bool printstats,
                    bool keep_trace);
        private:
            struct searchstate_t {
                size_t expandedStates = 0;
                size_t exploredStates = 1;
                std::vector<size_t> enabledTransitionsCount;
                size_t heurquery = 0;               
                bool usequeries;
            };
            
            template<typename Q, typename W = Structures::StateSet, typename G>
            void tryReach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                bool printstats);
            void printStats(searchstate_t& s, Structures::StateSetInterface*);
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, searchstate_t&, Structures::StateSetInterface*);
            ResultPrinter::Result printQuery(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result, searchstate_t&, Structures::StateSetInterface*);
            
            int _kbound;
            PetriNet& _net;
            size_t _satisfyingMarking = 0;
        };
        
        template<typename Q, typename W, typename G>
        void ReachabilitySearch::tryReach(   std::vector<std::shared_ptr<PQL::Condition> >& queries, 
                                        std::vector<ResultPrinter::Result>& results, bool usequeries,
                                        bool printstats)
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
            
            W states(_net, _kbound);    // stateset
            Q queue(&states);           // working queue
            G generator(_net, queries); // successor generator
            auto r = states.add(state);
            // this can fail due to reductions; we push tokens around and violate K
            if(r.first){ 
                // add initial to states, check queries on initial state
                _satisfyingMarking = r.second;
                // check initial marking
                if(ss.usequeries) 
                {
                    if(checkQueries(queries, results, working, ss, &states))
                    {
                        if(printstats) printStats(ss, &states);
                            return;
                    }
                }
                // add initial to queue
                {
                    PQL::DistanceContext dc(&_net, working.marking());
                    queue.push(r.second, dc, queries[ss.heurquery]);
                }

                // Search!
                while (queue.pop(state)) {
                    generator.prepare(&state);

                    while(generator.next(working)){
                        ss.enabledTransitionsCount[generator.fired()]++;
                        auto res = states.add(working);
                        if (res.first) {
                            {
                                PQL::DistanceContext dc(&_net, working.marking());
                                queue.push(res.second, dc, queries[ss.heurquery]);
                            }
                            states.setHistory(res.second, generator.fired());
                            _satisfyingMarking = res.second;
                            ss.exploredStates++;
                            if (checkQueries(queries, results, working, ss, &states)) {
                                if(printstats) printStats(ss, &states);
                                return;
                            }
                        }
                    }
                    ss.expandedStates++;
                }
            }

            // no more successors, print last results
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = printQuery(queries[i], i, ResultPrinter::NotSatisfied, ss, &states);                    
                }
            }            

            if(printstats) printStats(ss, &states);
        }

    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
