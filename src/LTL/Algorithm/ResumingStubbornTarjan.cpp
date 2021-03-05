/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "LTL/Algorithm/ResumingStubbornTarjan.h"

using PetriEngine::ReducingSuccessorGenerator;

namespace LTL {

    inline void _dump_state(const LTL::Structures::ProductState &state)
    {
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (size_t i = 1; i < state.size(); ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }


    bool ResumingStubbornTarjan::isSatisfied()
    {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        std::vector<State> initial_states;
        successorGenerator->makeInitialState(initial_states);
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            const auto res = seen.insertProductState(state);
            if (res.first) {
                push(state, res.second);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                if (!nexttrans(working, parent, dtop)) {
                    pop();
                    ++stats.expanded;
                    continue;
                }
                const auto[is_new, stateid] = seen.insertProductState(working);
                if (is_new) {
                    ++stats.explored;
                }
                //dtop.sucinfo.last_state = stateid;

                // lookup successor in 'hash' table
                auto p = chash[hash(stateid)];
                auto marking = seen.getMarkingId(stateid);
                while (p != std::numeric_limits<idx_t>::max() && cstack[p].stateid != stateid) {
                    p = cstack[p].next;
                    if (p != std::numeric_limits<idx_t>::max() && seen.getMarkingId(cstack[p].stateid) == marking) {
                        if (extstack.empty() || p >= extstack.top()) {
                            expandAll(dtop);
                        }
                    }
                }
                if (p != std::numeric_limits<idx_t>::max()) {
                    // we found the successor, i.e. there's a loop!
                    // now update lowlinks and check whether the loop contains an accepting state
                    update(p);
                    continue;
                }
                if (store.find(stateid) == std::end(store)) {
                    push(working, stateid);
                }
            }
        }
        return !violation;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    void ResumingStubbornTarjan::push(State &state, size_t stateid)
    {
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(stateid);
        cstack.emplace_back(ctop, stateid, chash[h]);
        chash[h] = ctop;
        dstack.push(DEntry{ctop, ReducingSuccessorGenerator::initial_suc_info});
        if (successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
    }


    void ResumingStubbornTarjan::pop()
    {
        const auto p = dstack.top().pos;
        dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                popCStack();
            }
        }
        if (!astack.empty() && p == astack.top()) {
            astack.pop();
        }
        if (!extstack.empty() && p == extstack.top()) {
            extstack.pop();
        }
        if (!dstack.empty()) {
            update(p);
        }
    }


    void ResumingStubbornTarjan::popCStack()
    {
        auto h = hash(cstack.back().stateid);
        store.insert(cstack.back().stateid);
        chash[h] = cstack.back().next;
        cstack.pop_back();
    }


    void ResumingStubbornTarjan::update(idx_t to)
    {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
        }
    }

    bool ResumingStubbornTarjan::nexttrans(State &state, State &parent, ResumingStubbornTarjan::DEntry &delem)
    {
        /*if (delem.sucinfo.tid >= net.numberOfTransitions() &&
            delem.sucinfo.tid < ReducingSuccessorGenerator::sucinfo::no_value) {
            return false;
        }*/
        CEntry &centry = cstack[delem.pos];
        seen.retrieveProductState(parent, centry.stateid);

        successorGenerator->prepare(&parent, delem.sucinfo);
        if (delem.sucinfo.hasEnabled()) {
            assert(delem.sucinfo.stubborn != std::numeric_limits<size_t>::max());
            assert(delem.sucinfo.buchi_state != std::numeric_limits<uint32_t>::max());
            _enabled.get(const_cast<bool *>(successorGenerator->stubborn()), delem.sucinfo.stubborn);
            _enabled.get(const_cast<bool *>(successorGenerator->enabled()), delem.sucinfo.enabled);
            state.setBuchiState(delem.sucinfo.buchi_state);
        }
        else {
            delem.sucinfo.enabled = _enabled.insert(successorGenerator->enabled()).second;
            delem.sucinfo.stubborn = _enabled.insert(successorGenerator->stubborn()).second;
        }
        auto res = successorGenerator->next(state, delem.sucinfo);
        return res;

/*        const bool *enabled;
        const bool *stubborn;

        if (!centry.hasEnabled()) {
            if (!successorGenerator->prepare(&parent)) {
                delem.sucinfo.tid = net.numberOfTransitions();
                centry.stubborn = 0;
                centry.enabled = 0;
                return successorGenerator->next(state);
                std::copy(parent.marking(), parent.marking() + net.numberOfPlaces() + 1,
                          state.marking());
                delem.sucinfo.tid = net.numberOfTransitions();
                centry.stubborn = 0;
                centry.enabled = 0;
            }
            centry.enabled = _enabled.insert(successorGenerator->enabled()).second;
            centry.stubborn = _enabled.insert(successorGenerator->stubborn()).second;
            //memcpy(buf1.get(), successorGenerator->stubborn(), net.numberOfTransitions());
            enabled = successorGenerator->enabled();
            stubborn = successorGenerator->stubborn();
        } else {
            _enabled.get(const_cast<bool *>(successorGenerator->enabled()), centry.enabled);
            _enabled.get(const_cast<bool *>(successorGenerator->stubborn()), centry.stubborn);
            enabled = successorGenerator->enabled();
            stubborn = successorGenerator->stubborn();
        }
        successorGenerator->prepare(&parent, delem.sucinfo);
        auto &tid = delem.sucinfo.tid;
        if (tid == ReducingSuccessorGenerator::sucinfo::no_value) tid = 0;
        while (tid < net.numberOfTransitions()) {
            if (enabled[tid] && stubborn[tid]) {
//                std::cerr << "tid: " << tid << "\tname: " << net.transitionNames()[tid] << std::endl;
                return successorGenerator->next(state, delem.sucinfo);
            }
            ++tid;
        }*/
        return false;
    }

    void LTL::ResumingStubbornTarjan::expandAll(DEntry &delem)
    {
#ifndef NDEBUG
#endif
        std::cerr << "Expanding all\n";
        CEntry &centry = cstack[delem.pos];
        _enabled.get(buf2.get(), delem.sucinfo.stubborn);
        _enabled.get(buf1.get(), delem.sucinfo.enabled);
        // tid is end while ntrans is end+1. Have to normalize like this since tid can be INT_MAX and thus overflow.
        assert(delem.sucinfo.tid != std::numeric_limits<uint32_t>::max());
        auto range = std::min(delem.sucinfo.tid, net.numberOfTransitions() - 1) + 1;
        for (uint32_t i = 0; i < delem.sucinfo.tid; ++i) {
            // unset previously fired transitions.
            if (buf2[i]) buf1[i] = false;
        }
        delem.sucinfo.stubborn = _enabled.insert(buf1.get()).second;
        delem.sucinfo.tid = 0;
        extstack.push(centry.stateid);
    }
}