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
#include "BreadthFirstReachabilitySearch.h"
#include "../PQL/PQL.h"
#include "../PQL/Contexts.h"
#include "../Structures/StateSet.h"

#include <list>
#include <string.h>

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine {
    namespace Reachability {

        ReachabilityResult BreadthFirstReachabilitySearch::reachable(PetriNet &net,
                const MarkVal *m0,
                PQL::Condition *query,
                size_t memorylimit) {
            //Create StateSet
            StateSet states(net, _kbound, memorylimit);

            State* state = new State();
            State* working = new State();
            state->setMarking(net.makeInitialMarking());
            working->setMarking(net.makeInitialMarking());
            states.add(state);

            BigInt expandedStates = 0;
            BigInt exploredStates = 1;
            std::vector<BigInt> enabledTransitionsCount(net.numberOfTransitions());

            if (query->evaluate(*state, &net)) {
                return ReachabilityResult(ReachabilityResult::Satisfied, "Query was satisfied",
                        expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound());
            }

            while (states.nextWaiting(state)) {

                net.reset(state);
                while(net.next(working)){
                    enabledTransitionsCount[net.fireing()]++;
                    if (states.add(working)) {
                        exploredStates++;
                        if (query->evaluate(*working, &net)) {
                            //ns->dumpTrace(net);
                            return ReachabilityResult(ReachabilityResult::Satisfied,
                                    "A state satisfying the query was found", expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound());
                        }
                    }
                }
                expandedStates++;
            }

            return ReachabilityResult(ReachabilityResult::NotSatisfied,
                    "No state satisfying the query exists.", expandedStates, exploredStates, states.discovered(), enabledTransitionsCount, states.maxTokens(), states.maxPlaceBound());
        }

    }
}
