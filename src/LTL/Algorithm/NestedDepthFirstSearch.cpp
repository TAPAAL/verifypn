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
#include "LTL/SuccessorGeneration/Spoolers.h"

namespace LTL {

    bool NestedDepthFirstSearch::check()
    {
        if(_heuristic)
        {
            SpoolingSuccessorGenerator gen(_net, _formula);
            EnabledSpooler spooler(_net, gen);
            gen.set_spooler(spooler);
            gen.set_heuristic(_heuristic);
            ProductSuccessorGenerator prod_gen(_net, _buchi, gen);
            dfs(prod_gen);
        } else {
            ResumingSuccessorGenerator gen(_net);
            ProductSuccessorGenerator prod_gen(_net, _buchi, gen);
            dfs(prod_gen);
        }
        return !_violation;
    }

    std::pair<bool,size_t> NestedDepthFirstSearch::mark(State& state, const uint8_t MARKER)
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
        }
        return std::make_pair(is_new, stateid);
    }

    template<typename T>
    void NestedDepthFirstSearch::dfs(ProductSuccessorGenerator<T>& successor_generator)
    {
        this->_is_weak = successor_generator.is_weak() && this->_shortcircuitweak;
        light_deque<stack_entry_t<T>> todo;
        light_deque<stack_entry_t<T>> nested_todo;

        State working = this->_factory.new_state();
        State curState = this->_factory.new_state();

        {
            std::vector<State> initial_states = successor_generator.make_initial_state();
            for (auto &state : initial_states) {
                auto res = _states.add(state);
                if (res.first) {
                    todo.push_back(stack_entry_t<T>{res.second, T::initial_suc_info()});
                }
            }
        }

        while (!todo.empty()) {
            auto &top = todo.back();
            _states.decode(curState, top._id);
            successor_generator.prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo._last_state);
            }
            if (!successor_generator.next(working, top._sucinfo)) {
                // no successor
                if (curState.is_accepting()) {
                    if(successor_generator.has_invariant_self_loop(curState))
                        _violation = true;
                    else
                        ndfs(successor_generator, curState, nested_todo);
                    if (_violation) {
                        if(_build_trace)
                            build_trace(todo, nested_todo);
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
                    if(successor_generator.is_accepting(curState) &&
                       successor_generator.has_invariant_self_loop(curState))
                    {
                        _violation = true;
                        if(_build_trace)
                            build_trace(todo, nested_todo);
                        return;
                    }
                    todo.push_back(stack_entry_t<T>{stateid, T::initial_suc_info()});
                }
            }
        }
    }

    template<typename T>
    void NestedDepthFirstSearch::ndfs(ProductSuccessorGenerator<T>& successor_generator, const State &state, light_deque<stack_entry_t<T>>& nested_todo)
    {

        State working = _factory.new_state();
        State curState = _factory.new_state();

        nested_todo.push_back(stack_entry_t<T>{_states.add(state).second, T::initial_suc_info()});

        while (!nested_todo.empty()) {
            auto &top = nested_todo.back();
            _states.decode(curState, top._id);
            successor_generator.prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo._last_state);
            }
            if (!successor_generator.next(working, top._sucinfo)) {
                nested_todo.pop_back();
            } else {
                if (this->_is_weak && !successor_generator.is_accepting(working)) {
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
                    nested_todo.push_back(stack_entry_t<T>{stateid, T::initial_suc_info()});
                }
            }
        }
    }

    void NestedDepthFirstSearch::print_stats(std::ostream &os) const
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << _states.discovered() << std::endl
                  << "\tmax tokens:                 " << _states.max_tokens() << std::endl
                  << "\texplored states:            " << _mark_count[MARKER1] << std::endl
                  << "\texplored states (nested):   " << _mark_count[MARKER2] << std::endl;
    }


    template<typename T>
    void NestedDepthFirstSearch::build_trace(light_deque<stack_entry_t<T>>& todo, light_deque<stack_entry_t<T>>& nested_todo)
    {
        size_t loop_id = std::numeric_limits<size_t>::max();
        // last element of todo-stack always has a "garbage" transition, it is the
        // current working element OR first element of nested.

        if(!todo.empty())
            todo.pop_back();
        if(!nested_todo.empty()) {
            // here the last state is significant
            // of the successor is the check that demonstrates the violation.
            loop_id = nested_todo.back()._id;
            nested_todo.pop_back();
        }

        for(auto* stck : {&todo, &nested_todo})
        {
            while(!(*stck).empty())
            {
                auto& top = (*stck).front();
                if(top._id == loop_id)
                {
                    _loop = _trace.size();
                    loop_id = std::numeric_limits<size_t>::max();
                }
                _trace.emplace_back(top._sucinfo.transition());
                (*stck).pop_front();
            }
        }
    }
}
