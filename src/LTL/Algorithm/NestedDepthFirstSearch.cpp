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

#include "LTL/Algorithm/NestedDepthFirstSearch.h"

namespace LTL {
    template<typename W>
    bool NestedDepthFirstSearch<W>::isSatisfied()
    {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        dfs();
        return !violation;
    }

    template<typename W>
    void NestedDepthFirstSearch<W>::dfs()
    {
        std::stack<size_t> call_stack;
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        {
            std::vector<State> initial_states = successorGenerator->makeInitialState();
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(StackEntry{res.second, LTL::ResumingSuccessorGenerator::initial_suc_info()});
                _discovered++;
            }
        }

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!successorGenerator->next(working, top.sucinfo)) {
                // no successor
                todo.pop();
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation) {
                        if constexpr (SaveTrace) {
                            std::stack<std::pair<size_t, size_t>> transitions;
                            size_t next = top.id;
                            states.decode(working, next);
                            while (!successorGenerator->isInitialState(working)) {
                                auto[parent, transition] = states.getHistory(next);
                                transitions.push(std::make_pair(next, transition));
                                next = parent;
                                states.decode(working, next);
                            }
                            printTrace(transitions);
                        }
                        return;
                    }
                }
            } else {
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark1.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    if constexpr (SaveTrace) {
                        states.setParent(top.id);
                        states.setHistory(stateid, successorGenerator->fired());
                    }
                    _discovered++;
                    todo.push(StackEntry{stateid, LTL::ResumingSuccessorGenerator::initial_suc_info()});
                }
            }
        }
    }

    template<typename W>
    void NestedDepthFirstSearch<W>::ndfs(State &state)
    {
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        todo.push(StackEntry{states.add(state).second, LTL::ResumingSuccessorGenerator::initial_suc_info()});

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!successorGenerator->next(working, top.sucinfo)) {
                todo.pop();
            } else {
                if (is_weak && !successorGenerator->isAccepting(working)) {
                    continue;
                }
                if (working == *seed) {
                    violation = true;
                    if constexpr (SaveTrace) {
                        auto[_, stateid] = states.add(working);
                        auto seedId = stateid;
                        states.setHistory2(stateid, successorGenerator->fired());
                        size_t next = stateid;
                        // follow trace until back at seed state.
                        do {
                            auto[state, transition] = states.getHistory2(next);
                            nested_transitions.push(std::make_pair(next, transition));
                            next = state;
                        } while (next != seedId);
                    }
                    return;
                }
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark2.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    if constexpr (SaveTrace) {
                        states.setParent(top.id);
                        states.setHistory2(stateid, successorGenerator->fired());
                    }
                    _discovered++;
                    todo.push(StackEntry{stateid, LTL::ResumingSuccessorGenerator::initial_suc_info()});
                }

            }
        }
    }

    template<typename W>
    void NestedDepthFirstSearch<W>::printStats(std::ostream &os)
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << states.discovered() << std::endl
                  << "\tmax tokens:                 " << states.maxTokens() << std::endl
                  << "\texplored states:            " << mark1.size() << std::endl
                  << "\texplored states (nested):   " << mark2.size() << std::endl;
    }


    template<typename W>
    void NestedDepthFirstSearch<W>::printTrace(std::stack<std::pair<size_t, size_t>> &transitions, std::ostream &os)
    {
        State state = factory.newState();
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
              "<trace>\n";
        while (!transitions.empty()) {
            auto[stateid, transition] = transitions.top();
            states.decode(state, stateid);
            printTransition(transition, state, os) << std::endl;
            transitions.pop();
        }
        printLoop(os);
        while (!nested_transitions.empty()) {
            auto[stateid, transition] = nested_transitions.top();
            states.decode(state, stateid);

            printTransition(transition, state, os) << std::endl;
            nested_transitions.pop();
        }
        os << std::endl << "</trace>" << std::endl;
    }

    template
    class NestedDepthFirstSearch<PetriEngine::Structures::StateSet>;

    template
    class NestedDepthFirstSearch<PetriEngine::Structures::TracableStateSet>;
}