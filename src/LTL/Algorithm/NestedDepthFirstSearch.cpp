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
    bool NestedDepthFirstSearch<S>::is_satisfied()
    {
        this->_is_weak = this->_successorGenerator->is_weak() && this->_shortcircuitweak;
        dfs();
        return !_violation;
    }

    template<typename S>
    std::pair<bool,size_t> NestedDepthFirstSearch<S>::mark(State& state, const uint8_t MARKER)
    {
        auto[_, stateid] = _states.add(state);
        if (stateid == std::numeric_limits<size_t>::max()) {
            return std::make_pair(false, stateid);
        }

        auto r = _markers[stateid];
        _markers[stateid] = (MARKER | r);
        const bool is_new = (r & MARKER) == 0;
        if(is_new)
        {
            ++_mark_count[MARKER];
            ++this->_discovered;
        }
        return std::make_pair(is_new, stateid);
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::dfs()
    {

        light_deque<stack_entry_t> todo;
        light_deque<stack_entry_t> nested_todo;

        State working = this->_factory.new_state();
        State curState = this->_factory.new_state();

        {
            std::vector<State> initial_states = this->_successorGenerator->make_initial_state();
            for (auto &state : initial_states) {
                auto res = _states.add(state);
                if (res.first) {
                    todo.push_back(stack_entry_t{res.second, S::initial_suc_info()});
                    ++this->_discovered;
                }
            }
        }

        while (!todo.empty()) {
            auto &top = todo.back();
            _states.decode(curState, top._id);
            this->_successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo._last_state);
            }
            if (!this->_successorGenerator->next(working, top._sucinfo)) {
                // no successor
                if (curState.is_accepting()) {
                    if(this->_successorGenerator->has_invariant_self_loop(curState))
                        _violation = true;
                    else
                        ndfs(curState, nested_todo);
                    if (_violation) {
                        return;
                    }
                }
                todo.pop_back();
            } else {
                auto [is_new, stateid] = mark(working, MARKER1);
                if (stateid == std::numeric_limits<size_t>::max()) {
                    continue;
                }
                top._sucinfo._last_state = stateid;
                if (is_new) {
                    ++this->_discovered;
                    if(this->_successorGenerator->is_accepting(curState) &&
                       this->_successorGenerator->has_invariant_self_loop(curState))
                    {
                        _violation = true;
                        return;
                    }
                    todo.push_back(stack_entry_t{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::ndfs(const State &state, light_deque<stack_entry_t>& nested_todo)
    {

        State working = this->_factory.new_state();
        State curState = this->_factory.new_state();

        nested_todo.push_back(stack_entry_t{_states.add(state).second, S::initial_suc_info()});

        while (!nested_todo.empty()) {
            auto &top = nested_todo.back();
            _states.decode(curState, top._id);
            this->_successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo._last_state);
            }
            if (!this->_successorGenerator->next(working, top._sucinfo)) {
                nested_todo.pop_back();
            } else {
                if (this->_is_weak && !this->_successorGenerator->is_accepting(working)) {
                    continue;
                }
                if (working == state) {
                    _violation = true;
                    return;
                }
                auto [is_new, stateid] = mark(working, MARKER2);
                if (stateid == std::numeric_limits<size_t>::max())
                    continue;
                top._sucinfo._last_state = stateid;
                if (is_new) {
                    nested_todo.push_back(stack_entry_t{stateid, S::initial_suc_info()});
                }
            }
        }
    }

   /* template<typename S>
    void NestedDepthFirstSearch<S>::printStats(std::ostream &os)
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << _states.discovered() << std::endl
                  << "\tmax tokens:                 " << _states.max_tokens() << std::endl
                  << "\texplored states:            " << _mark_count[MARKER1] << std::endl
                  << "\texplored states (nested):   " << _mark_count[MARKER2] << std::endl;
    }*/


    template<typename S>
    void NestedDepthFirstSearch<S>::print_trace(light_deque<stack_entry_t>& _todo, light_deque<stack_entry_t>& _nested_todo, std::ostream &os)
    {
        /*os << "<trace>\n";
        if(this->_reducer)
            this->_reducer->initFire(os);
        size_t loop_id = std::numeric_limits<size_t>::max();
        // last element of todo-stack always has a "garbage" transition, it is the
        // current working element OR first element of nested.

        if(!_todo.empty())
            _todo.pop_back();
        if(!_nested_todo.empty()) {
            // here the last state is significant
            // of the successor is the check that demonstrates the violation.
            loop_id = _nested_todo.back()._id;
            _nested_todo.pop_back();
        }

        for(auto* stck : {&_todo, &_nested_todo})
        {
            while(!(*stck).empty())
            {
                auto& top = (*stck).front();
                if(top._id == loop_id)
                {
                    this->printLoop(os);
                    loop_id = std::numeric_limits<size_t>::max();
                }
                this->printTransition(top._sucinfo.transition(), os) << std::endl;
                (*stck).pop_front();
            }
        }
        os << std::endl << "</trace>" << std::endl;*/
    }

    template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator>;

    template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator>;
}
