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
    template<typename S, typename W>
    bool NestedDepthFirstSearch<S, W>::isSatisfied()
    {
        this->is_weak = this->successorGenerator->is_weak() && this->shortcircuitweak;
        dfs();
        return !violation;
    }

    template<typename S, typename W>
    void NestedDepthFirstSearch<S, W>::dfs()
    {
        std::stack<size_t> call_stack;
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        {

            std::vector<State> initial_states = this->successorGenerator->makeInitialState();
            for (auto &state : initial_states) {
                auto res = states.add(state);
                if (res.first) {
                    assert(res.first);
                    todo.push(StackEntry{res.second, S::initial_suc_info()});
                    this->_discovered++;
                }
            }
        }

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            this->successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top.sucinfo)) {
                // no successor
                todo.pop();
                if (this->successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation) {
                        if constexpr (SaveTrace) {
                            std::stack<std::pair<size_t, size_t>> transitions;
                            size_t next = top.id;
                            states.decode(working, next);
                            while (!this->successorGenerator->isInitialState(working)) {
                                auto[parent, transition] = states.getHistory(next);
                                transitions.push(std::make_pair(parent, transition));
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
                if (stateid == std::numeric_limits<size_t>::max()) {
                    continue;
                }
                auto[it, is_new] = mark1.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    if constexpr (SaveTrace) {
                        states.setParent(top.id);
                        states.setHistory(stateid, this->successorGenerator->fired());
                    }
                    this->_discovered++;
                    todo.push(StackEntry{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S, typename W>
    void NestedDepthFirstSearch<S, W>::ndfs(State &state)
    {
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        todo.push(StackEntry{states.add(state).second, S::initial_suc_info()});

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            this->successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top.sucinfo)) {
                todo.pop();
            } else {
                if (this->is_weak && !this->successorGenerator->isAccepting(working)) {
                    continue;
                }
                if (working == *seed) {
                    violation = true;
                    if constexpr (SaveTrace) {
                        auto[_, stateid] = states.add(working);
                        auto seedId = stateid;
                        states.setHistory2(stateid, this->successorGenerator->fired());
                        size_t next = stateid;
                        // follow trace until back at seed state.
                        do {
                            auto[state, transition] = states.getHistory2(next);
                            nested_transitions.push(std::make_pair(state, transition));
                            next = state;
                        } while (next != seedId);
                    }
                    return;
                }
                auto[_, stateid] = states.add(working);
                if (stateid == std::numeric_limits<size_t>::max()) {
                    continue;
                }
                auto[it, is_new] = mark2.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    if constexpr (SaveTrace) {
                        states.setParent(top.id);
                        states.setHistory2(stateid, this->successorGenerator->fired());
                    }
                    this->_discovered++;
                    todo.push(StackEntry{stateid, S::initial_suc_info()});
                }

            }
        }
    }

    template<typename S, typename W>
    void NestedDepthFirstSearch<S, W>::printStats(std::ostream &os)
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << states.discovered() << std::endl
                  << "\tmax tokens:                 " << states.max_tokens() << std::endl
                  << "\texplored states:            " << mark1.size() << std::endl
                  << "\texplored states (nested):   " << mark2.size() << std::endl;
    }


    template<typename S, typename W>
    void NestedDepthFirstSearch<S, W>::printTrace(std::stack<std::pair<size_t, size_t>> &transitions, std::ostream &os)
    {
        State state = factory.newState();
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
              "<trace>\n";
        while (!transitions.empty()) {
            auto[stateid, transition] = transitions.top();
            states.decode(state, stateid);
            this->printTransition(transition, state, os) << std::endl;
            transitions.pop();
        }
        this->printLoop(os);
        while (!nested_transitions.empty()) {
            auto[stateid, transition] = nested_transitions.top();
            states.decode(state, stateid);

            this->printTransition(transition, state, os) << std::endl;
            nested_transitions.pop();
        }
        os << std::endl << "</trace>" << std::endl;
    }

    template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator, LTL::Structures::BitProductStateSet<>>;

    template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator, LTL::Structures::TraceableBitProductStateSet<>>;

    template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator, LTL::Structures::BitProductStateSet<>>;

    template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator, LTL::Structures::TraceableBitProductStateSet<>>;

}
