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
    template <bool SaveTrace>
    bool NestedDepthFirstSearch<SaveTrace>::isSatisfied() {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        dfs();
        return !violation;
    }

    template <bool SaveTrace>
    void NestedDepthFirstSearch<SaveTrace>::dfs() {
        std::stack<size_t> call_stack;
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        {
            std::vector<State> initial_states;
            successorGenerator->makeInitialState(initial_states);
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(StackEntry{res.second, initial_suc_info});
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
                            printStack(todo);
                        }
                        return;
                    }
                }
            } else {
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark1.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }
            }
        }
    }

    template <bool SaveTrace>
    void NestedDepthFirstSearch<SaveTrace>::ndfs(State &state) {
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        todo.push(StackEntry{states.add(state).second, initial_suc_info});

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
                        std::cout << "Violation found. Printing trace. Most recent state first." << std::endl;
                        std::cout << "Loop end." << std::endl;
                        printStack(todo);
                        std::cout << "Loop begin." << std::endl;
                    }
                    return;
                }
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark2.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }

            }
        }
    }

    inline void print_state(const LTL::Structures::ProductState &state, std::size_t nplaces=-1, std::ostream &os=std::cerr) {
        if (nplaces == -1) nplaces = state.size() -1;
        os << "marking: ";
        os << state.marking()[0];
        for (int i = 1; i <= nplaces; ++i) {
            os << ", " << state.marking()[i];
        }
        os << std::endl;
    }

    template<> void NestedDepthFirstSearch<true>::printStack(std::stack<StackEntry> &stack) {
        State state = factory.newState();
        while (!stack.empty()) {
            auto &top = stack.top();
            stack.pop();
            states.decode(state, top.id);
            print_state(state, -1, std::cout);
        }
    }
    template
    class NestedDepthFirstSearch<true>;

    template
    class NestedDepthFirstSearch<false>;
}