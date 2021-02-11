/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#include "LTL/Algorithm/RandomNDFS.h"

namespace LTL {

    bool RandomNDFS::isSatisfied() {
        dfs();
        return !violation;
    }

    void RandomNDFS::dfs() {
        std::stack<size_t> call_stack;

        PetriEngine::Structures::StateSet states{net, 0, (int) net.numberOfPlaces() + 1};
        PetriEngine::Structures::RDFSQueue todo{&states};

        State working = factory.newState();
        PQL::DistanceContext ctx{&net, working.marking()};
        State curState = factory.newState();
        {
            std::vector<State> initial_states;
            successorGenerator->makeInitialState(initial_states);
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(res.second, ctx, formula);
            }
        }

        while (todo.top(curState)) {
            if (!call_stack.empty() && states.lookup(curState).second == call_stack.top()) {
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation)
                        return;
                }
                todo.pop(curState);
                call_stack.pop();
            } else {
                ++stats.expanded;
                call_stack.push(states.add(curState).second);
                if (!mark1.add(curState).first) {
                    continue;
                }
                successorGenerator->prepare(&curState);
                while (successorGenerator->next(working)) {
                    ++stats.explored;
                    auto r = states.add(working);
                    todo.push(r.second, ctx, formula);
                }
            }
        }
    }


    void RandomNDFS::ndfs(State &state) {
        PetriEngine::Structures::StateSet states{net, 0, (int) net.numberOfPlaces() + 1};
        PetriEngine::Structures::RDFSQueue todo{&states};

        State working = factory.newState();
        State curState = factory.newState();

        PQL::DistanceContext ctx{&net, state.marking()};

        todo.push(states.add(state).second, ctx, formula);

        while (todo.pop(curState)) {
            ++stats.expanded;
            if (!mark2.add(curState).first) {
                continue;
            }

            successorGenerator->prepare(&curState);
            while (successorGenerator->next(working)) {
                ++stats.explored;
                if (working == *seed) {
                    violation = true;
                    return;
                }
                auto r = states.add(working);
                todo.push(r.second, ctx, formula);
            }
        }

    }

    void RandomNDFS::printStats(ostream &os) {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << mark1.discovered() << std::endl
                  << "\tdiscovered states (nested): " << mark2.discovered() << std::endl
                  << "\tmax tokens:                 " << std::max(mark1.maxTokens(), mark2.maxTokens()) << std::endl
                  << "\texplored states:            " << stats.explored << std::endl
                  << "\texplored states (nested):   " << stats.expanded << std::endl;
    }
}