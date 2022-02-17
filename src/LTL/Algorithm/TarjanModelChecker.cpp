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

namespace LTL {

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    bool TarjanModelChecker<S, G, SaveTrace, Spooler...>::check()
    {
        this->_is_weak = this->_successorGenerator->is_weak() && this->_shortcircuitweak;
        std::vector<State> initial_states = this->_successorGenerator->make_initial_state();
        State working = this->_factory.new_state();
        State parent = this->_factory.new_state();
        for (auto &state : initial_states) {
            const auto res = _seen.add(state);
            if (res.first) {
                push(state, res.second);
            }
            while (!_dstack.empty() && !_violation) {
                dentry_t &dtop = _dstack.top();
                // write next successor state to working.
                if (!next_trans(working, parent, dtop)) {
                    ++this->_expanded;
#ifndef NDEBUG
                    std::cerr << "backtrack\n";
#endif
                    pop();
                    continue;
                }
#ifndef NDEBUG
                auto fired =
#endif
                this->_successorGenerator->fired();
#ifndef NDEBUG
                if (fired >= std::numeric_limits<uint32_t>::max() - 3) {
                    std::cerr << "looping\n";
                }
                else {
                    //state.print(*this->net, std::cerr); std::cerr << ": <transition id='" << this->net->transitionNames()[fired] << "'>" << std::endl ;
                }
#endif
                ++this->_explored;
                const auto[isnew, stateid] = _seen.add(working);
                if (stateid == std::numeric_limits<idx_t>::max()) {
                    continue;
                }

                if constexpr (SaveTrace) {
                    if (isnew) {
                        _seen.setHistory(stateid, this->_successorGenerator->fired());
                    }
                }

                dtop._sucinfo._last_state = stateid;

                // lookup successor in 'hash' table
                auto suc_pos = _chash[hash(stateid)];
                auto marking = _seen.get_marking_id(stateid);
                while (suc_pos != std::numeric_limits<idx_t>::max() && _cstack[suc_pos]._stateid != stateid) {
                    if constexpr (_is_spooling) {
                        if (_cstack[suc_pos]._dstack && _seen.get_marking_id(_cstack[suc_pos]._stateid) == marking) {
                            this->_successorGenerator->generate_all(&parent, dtop._sucinfo);
                        }
                    }
                    suc_pos = _cstack[suc_pos]._next;
                }
                if (suc_pos != std::numeric_limits<idx_t>::max()) {
                    if constexpr (_is_spooling) {
                        if (_cstack[suc_pos]._dstack) {
                            this->_successorGenerator->generate_all(&parent, dtop._sucinfo);
                        }
                    }
                    // we found the successor, i.e. there's a loop!
                    // now update lowlinks and check whether the loop contains an accepting state
                    update(suc_pos);
                    continue;
                }
                if (_store.find(stateid) == std::end(_store)) {
                    push(working, stateid);
                }
            }
            if constexpr (SaveTrace) {
                // print counter-example if it exists.
                if (_violation) {
                    std::stack<dentry_t> revstack;
                    while (!_dstack.empty()) {
                        revstack.push(std::move(_dstack.top()));
                        _dstack.pop();
                    }
                    print_trace(std::move(revstack));
                    return false;
                }
            }
        }
        return !_violation;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    void TarjanModelChecker<S, G, SaveTrace, Spooler...>::push(State &state, size_t stateid) {
        const auto ctop = static_cast<idx_t>(_cstack.size());
        const auto h = hash(stateid);
        _cstack.emplace_back(ctop, stateid, _chash[h]);
        _chash[h] = ctop;
        _dstack.push(dentry_t{ctop});
        if (this->_successorGenerator->is_accepting(state)) {
            _astack.push(ctop);
            if (this->_successorGenerator->has_invariant_self_loop(state)){
                _violation = true;
                _invariant_loop = true;
            }
        }
        if constexpr (_is_spooling) {
            this->_successorGenerator->push();
        }
    }

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    void TarjanModelChecker<S, G, SaveTrace, Spooler...>::pop()
    {
        const auto p = _dstack.top()._pos;
        _dstack.pop();
        _cstack[p]._dstack = false;
        if (_cstack[p]._lowlink == p) {
            while (_cstack.size() > p) {
                popCStack();
            }
        } else if (this->_is_weak) {
            State state = this->_factory.new_state();
            _seen.decode(state, _cstack[p]._stateid);
            if (!this->_successorGenerator->is_accepting(state)) {
                popCStack();
            }
        }
        if (!_astack.empty() && p == _astack.top()) {
            _astack.pop();
        }
        if (!_dstack.empty()) {
            update(p);
            if constexpr (_is_spooling) {
                this->_successorGenerator->pop(_dstack.top()._sucinfo);
            }
        }
    }

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    void TarjanModelChecker<S, G, SaveTrace, Spooler...>::popCStack()
    {
        auto h = hash(_cstack.back()._stateid);
        _store.insert(_cstack.back()._stateid);
        _chash[h] = _cstack.back()._next;
        _cstack.pop_back();
    }

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    void TarjanModelChecker<S, G, SaveTrace, Spooler...>::update(idx_t to)
    {
        const auto from = _dstack.top()._pos;
        assert(_cstack[to]._lowlink != std::numeric_limits<idx_t>::max() && _cstack[from]._lowlink != std::numeric_limits<idx_t>::max());
        if (_cstack[to]._lowlink <= _cstack[from]._lowlink) {
            // we have now found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component precedes an accepting state,
            // the found loop is accepting and thus a violation.
            _violation = (!_astack.empty() && to <= _astack.top());
            // either way update the component ID of the state we came from.
            _cstack[from]._lowlink = _cstack[to]._lowlink;
            if constexpr (SaveTrace) {
                _loop_state = _cstack[to]._stateid;
                _loop_trans = this->_successorGenerator->fired();
                _cstack[from]._lowsource = to;

            }
        }
    }

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    bool TarjanModelChecker<S, G, SaveTrace, Spooler...>::next_trans(State &state, State &parent, TarjanModelChecker::dentry_t &delem)
    {
        _seen.decode(parent, _cstack[delem._pos]._stateid);
        this->_successorGenerator->prepare(&parent, delem._sucinfo);
        // ensure that `state` buffer contains the correct state for Büchi successor generation.
        if (delem._sucinfo.has_prev_state()) {
            _seen.decode(state, delem._sucinfo._last_state);
        }
        auto res = this->_successorGenerator->next(state, delem._sucinfo);
        return res;
    }

    template<template<typename, typename...> typename S, typename G, bool SaveTrace, typename... Spooler>
    void TarjanModelChecker<S, G, SaveTrace, Spooler...>::print_trace(std::stack<dentry_t> &&dstack, std::ostream &os)
    {
        /*
        if constexpr (!SaveTrace) {
            return;
        } else {
            assert(_violation);
            os << "<trace>\n";
            this->_reducer->initFire(os);
            if (_cstack[dstack.top()._pos]._stateid == _loop_state)
                this->printLoop(os);
            dstack.pop();
            unsigned long p;
            bool had_deadlock = false;
            // print (reverted) dstack
            while (!dstack.empty()) {
                p = dstack.top()._pos;
                dstack.pop();
                auto stateid = _cstack[p]._stateid;
                auto[parent, tid] = _seen.getHistory(stateid);
                this->printTransition(tid, os) << '\n';
                if(tid >= std::numeric_limits<ptrie::uint>::max() - 1)
                {
                    had_deadlock = true;
                    break;
                }
                if(_cstack[p]._stateid == _loop_state)
                    this->printLoop(os);
                _cstack[p]._lowlink = std::numeric_limits<idx_t>::max();
            }
            // follow previously found back edges via lowsource until back in dstack.
            if(_cstack[p]._lowsource != std::numeric_limits<idx_t>::max() && !had_deadlock)
            {
                p = _cstack[p]._lowsource;
                while (_cstack[p]._lowlink != std::numeric_limits<idx_t>::max()) {
                    auto[parent, tid] = _seen.getHistory(_cstack[p]._stateid);
                    this->printTransition(tid, os) << '\n';
                    if(tid >= std::numeric_limits<ptrie::uint>::max() - 1)
                    {
                        had_deadlock = true;
                        break;
                    }
                    assert(_cstack[p]._lowsource != std::numeric_limits<idx_t>::max());
                    p = _cstack[p]._lowsource;
                }
                if(!had_deadlock)
                    this->printTransition(_loop_trans, os) << '\n';
            }

            os << "</trace>" << std::endl;
        }*/
    }

    template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::ResumingSuccessorGenerator, true>;

    template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::ResumingSuccessorGenerator, false>;

    template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true>;

    template
    class TarjanModelChecker<ProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false>;

    template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true, VisibleLTLStubbornSet>;

    template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false, VisibleLTLStubbornSet>;

    template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, true, EnabledSpooler>;

    template
    class TarjanModelChecker<ReachStubProductSuccessorGenerator, LTL::SpoolingSuccessorGenerator, false, EnabledSpooler>;
}
