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
#include "LTL/SuccessorGeneration/CompoundGenerator.h"
#include "LTL/Structures/CompoundStateSet.h"

namespace LTL {

    bool NestedDepthFirstSearch::check()
    {
        if(_heuristic)
        {
            if(_hyper_traces > 1)
                throw base_error("Hyper-LTL with heuristics or partial order reduction not yet enabled.");
            SpoolingSuccessorGenerator gen(_net, _formula);
            EnabledSpooler spooler(_net, gen);
            gen.set_spooler(spooler);
            gen.set_heuristic(_heuristic);
            return check_with_generator(gen);
        } else {
            if(_hyper_traces <= 1)
            {
                ResumingSuccessorGenerator gen(_net);
                return check_with_generator(gen);
            }
            else
            {
                CompoundGenerator gen(_net, _hyper_traces);
                return check_with_generator(gen);
            }
        }
    }

    template<typename G>
    bool NestedDepthFirstSearch::check_with_generator(G& gen) {
        ProductSuccessorGenerator prod_gen(_net, _buchi, gen);
        if constexpr (std::is_same<G,CompoundGenerator>::value) {
            LTL::Structures::CompoundStateSet<ptrie::map<Structures::stateid_t, uint8_t>> states(_net, _hyper_traces, _kbound);
            dfs(prod_gen, states);
            _discovered = states.discovered();
            _max_tokens = states.max_tokens();
            _configurations = states.configurations();
            _markings = states.markings();
        }
        else
        {
            LTL::Structures::BitProductStateSet<ptrie::map<Structures::stateid_t, uint8_t>> states(_net, _kbound);
            dfs(prod_gen, states);
            _discovered = states.discovered();
            _max_tokens = states.max_tokens();
            _configurations = states.configurations();
            _markings = states.markings();
        }
        return !_violation;
    }

    template<typename S>
    std::pair<bool,size_t> NestedDepthFirstSearch::mark(S& states, State& state, const uint8_t MARKER)
    {
        // technically we could decorate the states here instead of
        // maintaining the index twice in the _mark_count.
        // this would also spare us one ptrie lookup.
        auto[_, stateid, data_id] = states.add(state);
        if (stateid == std::numeric_limits<size_t>::max()) {
            return std::make_pair(false, stateid);
        }

        auto& r = states.get_data(data_id);
        const bool is_new = (r & MARKER) == 0;
        if(is_new)
        {
            r = (MARKER | r);
            ++_mark_count[MARKER];
        }
        return std::make_pair(is_new, stateid);
    }

    template<typename T, typename S>
    void NestedDepthFirstSearch::dfs(ProductSuccessorGenerator<T>& successor_generator, S& states)
    {
        auto initial_states = successor_generator.make_initial_state();
        for (auto &state : initial_states) {
            auto res = states.add(state);
            if (std::get<0>(res)) {
                dfs(successor_generator, states, std::get<1>(res));
                if(_violation)
                    break;
            }
        }
    }

    template<typename T, typename S>
    void NestedDepthFirstSearch::dfs(ProductSuccessorGenerator<T>& successor_generator, S& states, size_t init)
    {
        light_deque<stack_entry_t<T>> todo;
        light_deque<stack_entry_t<T>> nested_todo;

        State working = this->_factory.new_state(_hyper_traces);
        State curState = this->_factory.new_state(_hyper_traces);

        todo.push_back(stack_entry_t<T>{init, successor_generator.initial_suc_info()});

        while (!todo.empty()) {
            auto &top = todo.back();
            states.decode(curState, top._id);
            successor_generator.prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                states.decode(working, top._sucinfo._last_state);
            }
            if (!successor_generator.next(working, top._sucinfo)) {
                // no successor
                if (curState.is_accepting()) {
                    if(successor_generator.has_invariant_self_loop(curState))
                        _violation = true;
                    else
                        ndfs(successor_generator, states, curState, nested_todo);
                    if (_violation) {
                        if(_build_trace)
                            build_trace(todo, nested_todo);
                        return;
                    }
                }
                todo.pop_back();
            } else {
                auto [is_new, stateid] = mark(states, working, MARKER1);
                if (stateid == std::numeric_limits<size_t>::max()) {
                    continue;
                }
                top._sucinfo._last_state = stateid;
                if (is_new) {
                    if(_shortcircuitweak &&
                       successor_generator.is_accepting(curState) &&
                       successor_generator.has_invariant_self_loop(curState))
                    {
                        _violation = true;
                        if(_build_trace)
                            build_trace(todo, nested_todo);
                        return;
                    }
                    todo.push_back(stack_entry_t<T>{stateid, successor_generator.initial_suc_info()});
                }
            }
        }
    }

    template<typename T, typename S>
    void NestedDepthFirstSearch::ndfs(ProductSuccessorGenerator<T>& successor_generator, S& states, const State &state, light_deque<stack_entry_t<T>>& nested_todo)
    {

        State working = _factory.new_state(_hyper_traces);
        State curState = _factory.new_state(_hyper_traces);

        nested_todo.push_back(stack_entry_t<T>{std::get<1>(states.add(state)), successor_generator.initial_suc_info()});

        while (!nested_todo.empty()) {
            auto &top = nested_todo.back();
            states.decode(curState, top._id);
            successor_generator.prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                states.decode(working, top._sucinfo._last_state);
            }
            if (!successor_generator.next(working, top._sucinfo)) {
                nested_todo.pop_back();
            } else {
                if(working.get_buchi_state() == state.get_buchi_state() &&
                   std::equal(working.marking(), working.marking() + _net.numberOfPlaces()*_hyper_traces,
                              state.marking())) {
                    _violation = true;
                    return;
                }
                auto [is_new, stateid] = mark(states, working, MARKER2);
                if (stateid == std::numeric_limits<size_t>::max())
                    continue;
                top._sucinfo._last_state = stateid;
                if (is_new) {
                    nested_todo.push_back(stack_entry_t<T>{stateid, successor_generator.initial_suc_info()});
                }
            }
        }
    }

    size_t NestedDepthFirstSearch::max_tokens() const {
        return _max_tokens;
    }

    size_t NestedDepthFirstSearch::get_markings() const {
        return _markings;
    }

    size_t NestedDepthFirstSearch::get_configurations() const {
        return _configurations;
    }

    size_t NestedDepthFirstSearch::get_discovered() const {
        return _discovered;
    }

    void NestedDepthFirstSearch::print_stats(std::ostream &os) const
    {
        ModelChecker::print_stats(os, _discovered, _max_tokens);
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
            loop_id = nested_todo.front()._id;
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
                auto res = top._sucinfo.transition();
                if constexpr (std::is_same<decltype(res),std::vector<uint32_t>>::value)
                {
                    _trace.emplace_back(top._sucinfo.transition());
                }
                else
                {
                    _trace.push_back({top._sucinfo.transition()});
                    assert(top._sucinfo.transition() < _net.numberOfTransitions());
                }
                (*stck).pop_front();
            }
        }
    }
}
