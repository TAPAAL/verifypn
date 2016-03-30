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
                size_t item;
                weighted_t(size_t w, size_t i) : weight(w), item(i) {};
                bool operator <(const weighted_t& y) const {
//                    if(weight == y.weight) return item < y.item;// do dfs if they match
                    if(weight == y.weight) return item > y.item;// do bfs if they match
                    return weight < y.weight;
                }
            };
            
            
            
            ReachabilitySearch(ResultPrinter& printer, int kbound = 0, Strategy strategy = BFS)
            : printer(printer) {
                _kbound = kbound;
                _strategy = strategy;
            }
            
            ~ReachabilitySearch()
            {
                delete states;
            }

            /** Perform reachability check using BFS with hasing */
            void reachable(PetriNet &net,
                    const MarkVal *m0,
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    size_t memorylimit,
                    std::vector<ResultPrinter::Result>& results,
                    bool printstats = false);
        private:
            void tryReach(PetriNet &net,
                const MarkVal *m0,
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                size_t memorylimit,
                std::vector<ResultPrinter::Result>& results);
            void printStats(PetriNet &net);
            
            bool nextWaiting(Structures::State* state);
            void pushOnQueue(   size_t id, Structures::State* state,
                                PQL::Condition* query,
                                PetriNet *net);
            

            int _kbound;
            BigInt expandedStates = 0;
            BigInt exploredStates = 1;
            Structures::StateSet* states = NULL;
            std::vector<BigInt> enabledTransitionsCount;
            Strategy _strategy;
            std::priority_queue<weighted_t> _queue;
            size_t _heurquery = 0;
        };

    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
