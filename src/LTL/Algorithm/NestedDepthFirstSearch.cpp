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
    template <typename W>
    bool NestedDepthFirstSearch<W>::isSatisfied() {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        dfs();
        return !violation;
    }

    template <typename W>
    void NestedDepthFirstSearch<W>::dfs() {
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
                        if constexpr (std::is_same_v<W, PetriEngine::Structures::TracableStateSet>) {
                            std::stack<size_t> transitions;
                            size_t next = top.id;
                            while (next != 0) {
                                auto[parent, transition] = states.getHistory(next);
                                next = parent;
                                transitions.push(transition);
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
                    if constexpr (std::is_same_v<W, PetriEngine::Structures::TracableStateSet>) {
                        states.setParent(top.id);
                        states.setHistory(stateid, successorGenerator->fired());
                    }
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }
            }
        }
    }

    template <typename W>
    void NestedDepthFirstSearch<W>::ndfs(State &state) {
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
                    if constexpr (std::is_same_v<W, PetriEngine::Structures::TracableStateSet>) {
                        auto[_, stateid] = states.add(working);
                        states.setHistory2(stateid, successorGenerator->fired());
                        size_t next = stateid;
                        ptrie::uint trans = 0;
                        while (trans < std::numeric_limits<ptrie::uint>::max() - 1) {
                            auto[state, transition] = states.getHistory2(next);
                            if (transition >= std::numeric_limits<ptrie::uint>::max() - 1){
                                auto[_, transition] = states.getHistory(next);
                                nested_transitions.push(transition);
                                break;
                            }
                            next = state;
                            trans = transition;
                            nested_transitions.push(transition);
                        }
                    }
                    return;
                }
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark2.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    if constexpr (std::is_same_v<W, PetriEngine::Structures::TracableStateSet>) {
                        states.setParent(top.id);
                        states.setHistory2(stateid, successorGenerator->fired());
                    }
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }

            }
        }
    }

    template<typename W>
    ostream &NestedDepthFirstSearch<W>::printTransition(size_t transition, uint indent, ostream &os) {
        if (transition == std::numeric_limits<ptrie::uint>::max() - 1) {
            os << std::string(indent, '\t') << "<deadlock/>";
            return os;
        }
        std::string tname = states.net().transitionNames()[transition];
        os << std::string(indent, '\t') << "<transition id=\"" << tname << "\" index=\"" << transition << "\"/>";
        return os;
    }

    template<typename W>
    void NestedDepthFirstSearch<W>::printTrace(std::stack<size_t> &transitions) {
        std::cerr << "Trace:\n<trace>\n";
        while(!transitions.empty()){
            printTransition(transitions.top(), 1) << std::endl;
            transitions.pop();
        }
        std::cerr << "\t<loop>" << std::endl;
        while(!nested_transitions.empty()){
            printTransition(nested_transitions.top(), 2) << std::endl;
            nested_transitions.pop();
        }
        std::cerr << "\t</loop>" << std::endl << "</trace>" << std::endl;
    }


    template class NestedDepthFirstSearch<PetriEngine::Structures::StateSet>;
    template class NestedDepthFirstSearch<PetriEngine::Structures::TracableStateSet>;
}