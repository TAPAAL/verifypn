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
    template<typename S>
    bool NestedDepthFirstSearch<S>::isSatisfied()
    {
        this->is_weak = this->successorGenerator->is_weak() && this->shortcircuitweak;
        dfs();
        return !_violation;
    }

    template<typename S>
    std::pair<bool,size_t> NestedDepthFirstSearch<S>::mark(State& state, const uint8_t MARKER)
    {
        auto[_, stateid] = _states.add(state);
        if(_markers.size() <= stateid)
            _markers.resize(stateid + 1);
        auto r = _markers[stateid];
        _markers[stateid] = (MARKER | r);
        const bool is_new = (r & MARKER) != 0;
        if(is_new)
            ++_mark_count[MARKER];
        return std::make_pair(is_new, stateid);
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::dfs()
    {

        light_deque<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        {

            std::vector<State> initial_states = this->successorGenerator->makeInitialState();
            for (auto &state : initial_states) {
                auto res = _states.add(state);
                assert(res.first);
                todo.push_back(StackEntry{res.second, S::initial_suc_info()});
                this->_discovered++;
            }
        }

        while (!todo.empty()) {
            auto &top = todo.back();
            _states.decode(curState, top._id);
            this->successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top._sucinfo)) {
                // no successor
                todo.pop_back();
                if (this->successorGenerator->isAccepting(curState)) {
                    _seed = &curState;
                    ndfs(curState);
                    if (_violation) {
                        if (_print_trace) {
                            /*std::stack<std::pair<size_t, size_t>> transitions;
                            size_t next = top._id;
                            _states.decode(working, next);
                            while (!this->successorGenerator->isInitialState(working)) {
                                auto[parent, transition] = _states.getHistory(next);
                                transitions.push(std::make_pair(parent, transition));
                                next = parent;
                                _states.decode(working, next);
                            }
                            printTrace(transitions);*/
                        }
                        return;
                    }
                }
            } else {
                auto [is_new, stateid] = mark(working, MARKER1);
                top._sucinfo.last_state = stateid;
                if (is_new) {
                    ++this->_discovered;
                    todo.push_back(StackEntry{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::ndfs(State &state)
    {
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        todo.push(StackEntry{_states.add(state).second, S::initial_suc_info()});

        while (!todo.empty()) {
            auto &top = todo.top();
            _states.decode(curState, top._id);
            this->successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top._sucinfo)) {
                todo.pop();
            } else {
                if (this->is_weak && !this->successorGenerator->isAccepting(working)) {
                    continue;
                }
                if (working == *_seed) {
                    _violation = true;
                    if (_print_trace) {
                        /*auto[_, stateid] = _states.add(working);
                        auto seedId = stateid;
                        _states.setHistory2(stateid, this->successorGenerator->fired());
                        size_t next = stateid;
                        // follow trace until back at seed state.
                        do {
                            auto[state, transition] = _states.getHistory2(next);
                            _nested_transitions.push(std::make_pair(state, transition));
                            next = state;
                        } while (next != seedId);*/
                    }
                    return;
                }
                auto [is_new, stateid] = mark(working, MARKER2);
                top._sucinfo.last_state = stateid;
                if (is_new) {
                    this->_discovered++;
                    todo.push(StackEntry{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::printStats(std::ostream &os)
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << _states.discovered() << std::endl
                  << "\tmax tokens:                 " << _states.maxTokens() << std::endl
                  << "\texplored states:            " << _mark_count[MARKER1] << std::endl
                  << "\texplored states (nested):   " << _mark_count[MARKER2] << std::endl;
    }


    template<typename S>
    void NestedDepthFirstSearch<S>::printTrace(std::stack<std::pair<size_t, size_t>> &transitions, std::ostream &os)
    {
        State state = factory.newState();
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
              "<trace>\n";
        while (!transitions.empty()) {
            auto[stateid, transition] = transitions.top();
            _states.decode(state, stateid);
            this->printTransition(transition, state, os) << std::endl;
            transitions.pop();
        }
        this->printLoop(os);
        while (!_nested_transitions.empty()) {
            auto[stateid, transition] = _nested_transitions.top();
            _states.decode(state, stateid);

            this->printTransition(transition, state, os) << std::endl;
            _nested_transitions.pop();
        }
        os << std::endl << "</trace>" << std::endl;
    }

    template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator>;

    template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator>;
}
