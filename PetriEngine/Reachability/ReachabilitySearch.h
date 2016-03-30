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
#include <queue>

#include "../Structures/State.h"
#include "ReachabilityResult.h"
#include "../PQL/PQL.h"
#include "../PetriNet.h"
#include "../Structures/StateSet.h"

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
            struct weighted_t {
                uint32_t weight;
                uint32_t item;
                weighted_t(uint32_t w, uint32_t i) : weight(w), item(i) {};
                bool operator <(const weighted_t& y) const {
//                    if(weight == y.weight) return item < y.item;// do dfs if they match
                    if(weight == y.weight) return item > y.item;// do bfs if they match
                    return weight < y.weight;
                }
            };
            
            
            
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
                Strategy strategy;
                size_t heurquery = 0;
                std::priority_queue<weighted_t> queue;
                bool usequeries;
            };
            
            void tryReach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                Strategy strategy,
                bool statespacesearch,
                bool printstats);
            void printStats(searchstate_t& s);
            bool checkQueries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                std::vector<ResultPrinter::Result>&,
                                Structures::State&, searchstate_t& );
            ResultPrinter::Result printQuery(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result, searchstate_t& );
            bool nextWaiting(Structures::State& state, searchstate_t& );
            void pushOnQueue(   size_t id, Structures::State& state,
                                std::shared_ptr<PQL::Condition>& query, searchstate_t& );
            

            int _kbound;
            Structures::StateSet states;
            PetriNet& _net;
        };

    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
