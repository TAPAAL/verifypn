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

#include "LTL/Algorithm/TarjanModelChecker.h"
#include "PetriEngine/PQL/PredicateCheckers.h"

namespace LTL {

    void TarjanModelChecker::print_stats(std::ostream &os) const {
        ModelChecker::print_stats(os, _discoverd, _max_tokens);
    }

    void TarjanModelChecker::set_partial_order(LTLPartialOrder o)
    {
        if(_net.has_inhibitor())
        {
            _order = LTLPartialOrder::None;
            return; // no partial order supported
        }
        if(PetriEngine::PQL::containsNext(_formula) && o == LTLPartialOrder::Visible)
        {
            _order = LTLPartialOrder::None;
            return; // also no POR
        }
        _order = o;
    }

    bool TarjanModelChecker::check() {
        if(_heuristic != nullptr || _order != LTLPartialOrder::None)
        {
            // we need advanced successor generator pipeline (we need to look at successors)
            std::unique_ptr<SuccessorSpooler> spooler;
            SpoolingSuccessorGenerator gen{_net, _formula};
            if (_order == LTLPartialOrder::Visible) {
                spooler = std::make_unique<VisibleLTLStubbornSet>(_net, _formula);
            } else if (_order == LTLPartialOrder::Liebke) {
                spooler = std::make_unique<AutomatonStubbornSet>(_net, _buchi);
            } else {
                spooler = std::make_unique<EnabledSpooler>(_net, gen);
            }

            gen.set_spooler(*spooler);

            if(_heuristic)
                gen.set_heuristic(_heuristic);

            if(_order == LTLPartialOrder::Automaton)
            {
                ReachStubProductSuccessorGenerator succ_gen(_net, _buchi, gen, std::make_unique<EnabledSpooler>(_net, gen));
                return select_trace_compute(succ_gen);
            }
            else {
                ProductSuccessorGenerator succ_gen(_net, _buchi, gen);
                return select_trace_compute(succ_gen);
            }
        }
        else
        {
            ResumingSuccessorGenerator gen{_net};
            ProductSuccessorGenerator succ_gen(_net, _buchi, gen);
            return select_trace_compute(succ_gen);
        }
    }

    template<typename SuccGen>
    bool TarjanModelChecker::select_trace_compute(SuccGen& successorGenerator)
    {
        return _build_trace ?
            compute<true, SuccGen>(successorGenerator) :
            compute<false, SuccGen>(successorGenerator);
    }


    template<bool SaveTrace, typename SuccGen>
    bool TarjanModelChecker::compute(SuccGen& successorGenerator)
    {

        using StateSet = std::conditional_t<SaveTrace, LTL::Structures::TraceableBitProductStateSet<>,
                LTL::Structures::BitProductStateSet<>>;
        using centry_t = std::conditional_t<SaveTrace,
                tracable_centry_t,
                plain_centry_t>;

        StateSet seen(_net, _k_bound);
        // master list of state information.
        light_deque<centry_t> cstack;
        // depth-first search stack, contains current search path.
        light_deque<dentry_t<SuccGen>> dstack;

        auto initial_states = successorGenerator.make_initial_state();
        State working = _factory.new_state();
        State parent = _factory.new_state();
        for (auto &state : initial_states) {
            if(_violation) break;
            const auto res = seen.add(state);
            if (std::get<0>(res)) {
                push(seen, cstack, dstack, successorGenerator, state, std::get<1>(res));
            }
            while (!dstack.empty() && !_violation) {
                auto &dtop = dstack.back();
                // write next successor state to working.
                if (!next_trans(seen, cstack, successorGenerator, working, parent, dtop)) {
                    ++_expanded;
#ifndef NDEBUG
                    std::cerr << "backtrack\n";
#endif
                    pop(seen, cstack, dstack, successorGenerator);
                    continue;
                }
#ifndef NDEBUG
                auto fired =
#endif
                successorGenerator.fired();
#ifndef NDEBUG
                if (fired >= std::numeric_limits<uint32_t>::max() - 3) {
                    std::cerr << "looping\n";
                }
#endif
                ++_explored;
                const auto[isnew, stateid, _] = seen.add(working);
                if (stateid == std::numeric_limits<idx_t>::max()) {
                    continue;
                }

                if constexpr (SaveTrace) {
                    if (isnew) {
                        seen.set_history(stateid, successorGenerator.fired());
                    }
                }

                dtop._sucinfo._last_state = stateid;

                // lookup successor in 'hash' table
                auto marking = StateSet::get_marking_id(stateid);
                auto suc_pos = _chash[hash(marking, StateSet::get_buchi_state(stateid))];
                while (suc_pos != std::numeric_limits<idx_t>::max() && cstack[suc_pos]._stateid != stateid) {
                    if constexpr (std::is_same<SuccGen, SpoolingSuccessorGenerator>::value) {
                        if (cstack[suc_pos]._dstack && StateSet::get_marking_id(cstack[suc_pos]._stateid) == marking) {
                            successorGenerator->generate_all(&parent, dtop._sucinfo);
                        }
                    }
                    suc_pos = cstack[suc_pos]._next;
                }
                if (suc_pos != std::numeric_limits<idx_t>::max()) {
                    if constexpr (std::is_same<SuccGen, SpoolingSuccessorGenerator>::value) {
                        if (cstack[suc_pos]._dstack) {
                            successorGenerator.generate_all(&parent, dtop._sucinfo);
                        }
                    }
                    // we found the successor, i.e. there's a loop!
                    // now update lowlinks and check whether the loop contains an accepting state
                    update(cstack, dstack, successorGenerator, suc_pos);
                    continue;
                }
                if (!_store.exists(stateid).first) {
                    auto bstate = StateSet::get_buchi_state(stateid);
                    if(_shortcircuitweak &&
                       successorGenerator.is_accepting(bstate) &&
                       successorGenerator.has_invariant_self_loop(bstate))
                    {
                        _violation = true;
                        break;
                    }
                    push(seen, cstack, dstack, successorGenerator, working, stateid);
                }
            }
            if constexpr (SaveTrace) {
                // print counter-example if it exists.
                if (_violation) {
                    light_deque<dentry_t<SuccGen>> revstack;
                    while (!dstack.empty()) {
                        revstack.push_back(std::move(dstack.back()));
                        dstack.pop_back();
                    }
                    build_trace(seen, std::move(revstack), cstack);
                }
            }
        }
        _discoverd = seen.discovered();
        _max_tokens = seen.max_tokens();
        return !_violation;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    template<typename StateSet, typename T, typename D, typename S>
    void TarjanModelChecker::push(StateSet& s, light_deque<T>& cstack, light_deque<D>& dstack, S& successor_generator, State &state, size_t stateid) {
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(StateSet::get_marking_id(stateid), StateSet::get_buchi_state(stateid));
        cstack.push_back(T{ctop, stateid, _chash[h]});
        _chash[h] = ctop;
        dstack.push_back(D{ctop});
        if (successor_generator.is_accepting(state)) {
            _astack.push_back(ctop);
            if (successor_generator.has_invariant_self_loop(state)){
                _violation = true;
                _invariant_loop = true;
            }
        }
        if constexpr (std::is_same<S, SpoolingSuccessorGenerator>::value) {
            successor_generator.push();
        }
    }

    template<typename S, typename T, typename D, typename SuccGen>
    void TarjanModelChecker::pop(S& seen, light_deque<T>& cstack, light_deque<D>& dstack, SuccGen& successorGenerator)
    {
        const auto p = dstack.back()._pos;
        dstack.pop_back();
        cstack[p]._dstack = false;
        if (cstack[p]._lowlink == p) {
            while (cstack.size() > p) {
                popCStack(seen, cstack);
            }
        }
        if (!_astack.empty() && p == _astack.back()) {
            _astack.pop_back();
        }
        if (!dstack.empty()) {
            update(cstack, dstack, successorGenerator, p);
            if constexpr (std::is_same<SuccGen, SpoolingSuccessorGenerator>::value) {
                successorGenerator.pop(dstack.back()._sucinfo);
            }
        }
    }

    template<typename StateSet, typename T>
    void TarjanModelChecker::popCStack(StateSet& s, light_deque<T>& cstack)
    {
        auto h = hash(StateSet::get_marking_id(cstack.back()._stateid), StateSet::get_buchi_state(cstack.back()._stateid));
        _store.insert(cstack.back()._stateid);
        _chash[h] = cstack.back()._next;
        cstack.pop_back();
    }


    template<typename T, typename D, typename SuccGen>
    void TarjanModelChecker::update(light_deque<T>& cstack, light_deque<D>& dstack, SuccGen& successorGenerator, idx_t to)
    {
        const auto from = dstack.back()._pos;
        assert(cstack[to]._lowlink != std::numeric_limits<idx_t>::max() && cstack[from]._lowlink != std::numeric_limits<idx_t>::max());
        if (cstack[to]._lowlink <= cstack[from]._lowlink) {
            // we have now found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component precedes an accepting state,
            // the found loop is accepting and thus a violation.
            _violation = (!_astack.empty() && to <= _astack.back());
            // either way update the component ID of the state we came from.
            cstack[from]._lowlink = cstack[to]._lowlink;
            if constexpr (T::save_trace()) {
                _loop_state = cstack[to]._stateid;
                _loop_trans = successorGenerator.fired();
                cstack[from]._lowsource = to;
            }
        }
    }

    template<typename S, typename T, typename SuccGen, typename D>
    bool TarjanModelChecker::next_trans(S& seen, light_deque<T>& cstack, SuccGen& successorGenerator, State &state, State &parent, D &delem)
    {
        seen.decode(parent, cstack[delem._pos]._stateid);
        successorGenerator.prepare(&parent, delem._sucinfo);
        // ensure that `state` buffer contains the correct state for Büchi successor generation.
        if (delem._sucinfo.has_prev_state()) {
            seen.decode(state, delem._sucinfo._last_state);
        }
        auto res = successorGenerator.next(state, delem._sucinfo);
        return res;
    }

    template<typename S, typename D, typename C>
    void TarjanModelChecker::build_trace(S& seen, light_deque<D> &&dstack, light_deque<C>& cstack)
    {
        assert(_violation);
        if (cstack[dstack.back()._pos]._stateid == _loop_state)
            _loop = _trace.size();
        dstack.pop_back();
        unsigned long p;
        bool had_deadlock = false;
        // print (reverted) dstack
        while (!dstack.empty()) {
            p = dstack.back()._pos;
            dstack.pop_back();
            auto stateid = cstack[p]._stateid;
            auto[parent, tid] = seen.get_history(stateid);
            _trace.emplace_back(tid);
            if(tid >= std::numeric_limits<ptrie::uint>::max() - 1)
            {
                had_deadlock = true;
                break;
            }
            if(cstack[p]._stateid == _loop_state)
                _loop = _trace.size();
            cstack[p]._lowlink = std::numeric_limits<idx_t>::max();
        }
        // follow previously found back edges via lowsource until back in dstack.
        if(cstack[p]._lowsource != std::numeric_limits<idx_t>::max() && !had_deadlock)
        {
            p = cstack[p]._lowsource;
            while (cstack[p]._lowlink != std::numeric_limits<idx_t>::max()) {
                auto[parent, tid] = seen.get_history(cstack[p]._stateid);
                _trace.emplace_back(tid);
                if(tid >= std::numeric_limits<ptrie::uint>::max() - 1)
                {
                    had_deadlock = true;
                    break;
                }
                assert(cstack[p]._lowsource != std::numeric_limits<idx_t>::max());
                p = cstack[p]._lowsource;
            }
            if(!had_deadlock)
                _trace.emplace_back(_loop_trans);
        }
    }
}
