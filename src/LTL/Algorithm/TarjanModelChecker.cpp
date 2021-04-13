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

    inline void _dump_state(const LTL::Structures::ProductState &state)
    {
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (size_t i = 1; i < state.size(); ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }

    template<typename S, bool SaveTrace>
    bool TarjanModelChecker<S, SaveTrace>::isSatisfied()
    {
        this->is_weak = this->successorGenerator->is_weak() && this->shortcircuitweak;
        std::vector<State> initial_states = this->successorGenerator->makeInitialState();
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            const auto res = seen.add(state);
            if (res.first) {
                push(state, res.second);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                // write next successor state to working.
                if (!nexttrans(working, parent, dtop)) {
                    ++this->stats.expanded;
                    pop();
                    continue;
                }
                ++this->stats.explored;
                const auto[isnew, stateid] = seen.add(working);

                if constexpr (SaveTrace) {
                    if (isnew) {
                        seen.setHistory(stateid, this->successorGenerator->fired());
                    }
                }
                dtop.sucinfo.last_state = stateid;

                // lookup successor in 'hash' table
                auto suc_pos = chash[hash(stateid)];
                auto marking = seen.getMarkingId(stateid);
                while (suc_pos != std::numeric_limits<idx_t>::max() && cstack[suc_pos].stateid != stateid) {
                    if constexpr (IsSpooling) {
                        if (cstack[suc_pos].dstack && seen.getMarkingId(cstack[suc_pos].stateid) == marking) {
                            this->successorGenerator->prepare(&parent, dtop.sucinfo);
                            this->successorGenerator->generateAll(dtop.sucinfo);
                            extstack.push(cstack.size() - 1);
                        }
                    }
                    suc_pos = cstack[suc_pos].next;
                }
                if (suc_pos != std::numeric_limits<idx_t>::max()) {
                    if constexpr (IsSpooling) {
                        if (cstack[suc_pos].dstack) {
                            this->successorGenerator->prepare(&parent, dtop.sucinfo);
                            this->successorGenerator->generateAll(dtop.sucinfo);
                            extstack.push(cstack.size() - 1);
                        }
                    }
                    // we found the successor, i.e. there's a loop!
                    // now update lowlinks and check whether the loop contains an accepting state
                    update(suc_pos);
                    continue;
                }
                if (store.find(stateid) == std::end(store)) {
                    push(working, stateid);
                }
            }
            if constexpr (SaveTrace) {
                // print counter-example if it exists.
                if (violation) {
                    std::stack<DEntry> revstack;
                    while (!dstack.empty()) {
                        revstack.push(std::move(dstack.top()));
                        dstack.pop();
                    }
                    printTrace(std::move(revstack));
                    return false;
                }
            }
        }
        return !violation;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    template<typename S, bool SaveTrace>
    void TarjanModelChecker<S, SaveTrace>::push(State &state, size_t stateid) {
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(stateid);
        cstack.emplace_back(ctop, stateid, chash[h]);
        chash[h] = ctop;
        dstack.push(DEntry{ctop});
        if (this->successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
        if constexpr (IsSpooling) {
            this->successorGenerator->push();
        }
    }

    template<typename S, bool SaveTrace>
    void TarjanModelChecker<S, SaveTrace>::pop()
    {
        const auto p = dstack.top().pos;
        dstack.pop();
        cstack[p].dstack = false;
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                popCStack();
            }
        } else if (this->is_weak) {
            State state = factory.newState();
            seen.decode(state, cstack[p].stateid);
            if (!this->successorGenerator->isAccepting(state)) {
                popCStack();
            }
        }
        if (!astack.empty() && p == astack.top()) {
            astack.pop();
        }
        if (!extstack.empty() && p == extstack.top()) {
            extstack.pop();
        }
        if (!extstack.empty() && p == extstack.top()) {
            extstack.pop();
        }
        if (!dstack.empty()) {
            update(p);
            if constexpr (IsSpooling) {
                this->successorGenerator->pop(dstack.top().sucinfo);
            }
        }
    }

    template<typename S, bool SaveTrace>
    void TarjanModelChecker<S, SaveTrace>::popCStack()
    {
        auto h = hash(cstack.back().stateid);
        store.insert(cstack.back().stateid);
        chash[h] = cstack.back().next;
        cstack.pop_back();
    }

    template<typename S, bool SaveTrace>
    void TarjanModelChecker<S, SaveTrace>::update(idx_t to)
    {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have now found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component precedes an accepting state,
            // the found loop is accepting and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            // either way update the component ID of the state we came from.
            cstack[from].lowlink = cstack[to].lowlink;
            if constexpr (SaveTrace) {
                loopstate = cstack[to].stateid;
                looptrans = this->successorGenerator->fired();
                cstack[from].lowsource = to;

            }
        }
    }

    template<typename S, bool SaveTrace>
    bool TarjanModelChecker<S, SaveTrace>::nexttrans(State &state, State &parent, TarjanModelChecker::DEntry &delem)
    {
        seen.decode(parent, cstack[delem.pos].stateid);
        this->successorGenerator->prepare(&parent, delem.sucinfo);
        // ensure that `state` buffer contains the correct state for Büchi successor generation.
        if (delem.sucinfo.has_prev_state()) {
            seen.decode(state, delem.sucinfo.last_state);
        }
        auto res = this->successorGenerator->next(state, delem.sucinfo);
        return res;
    }

    template<typename S, bool SaveTrace>
    void TarjanModelChecker<S, SaveTrace>::printTrace(std::stack<DEntry> &&dstack, std::ostream &os)
    {
        if constexpr (!SaveTrace) {
            return;
        } else {
            State state = factory.newState();
            os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                  "<trace>\n";
            if (cstack[dstack.top().pos].stateid == loopstate) this->printLoop(os);
            cstack[dstack.top().pos].lowlink = std::numeric_limits<idx_t>::max();
            dstack.pop();
            unsigned long p;
            // print (reverted) dstack
            while (!dstack.empty()) {
                p = dstack.top().pos;
                auto stateid = cstack[p].stateid;
                auto[parent, tid] = seen.getHistory(stateid);
                seen.decode(state, stateid);

                this->printTransition(tid, state, os) << '\n';
                if (stateid == loopstate) this->printLoop(os);

                cstack[p].lowlink = std::numeric_limits<idx_t>::max();
                dstack.pop();
            }
            // follow previously found back edges via lowsource until back in dstack.
            assert(cstack[p].lowsource != std::numeric_limits<idx_t>::max());
            p = cstack[p].lowsource;
            while (cstack[p].lowlink != std::numeric_limits<idx_t>::max()) {
                auto[parent, tid] = seen.getHistory(cstack[p].stateid);
                seen.decode(state, cstack[p].stateid);
                this->printTransition(tid, state, os) << '\n';
                assert(cstack[p].lowsource != std::numeric_limits<idx_t>::max());
                p = cstack[p].lowsource;
            }
            seen.decode(state, loopstate);
            this->printTransition(looptrans, state, os) << '\n';

            os << "</trace>" << std::endl;
        }
    }

    template
    class TarjanModelChecker<LTL::ResumingSuccessorGenerator, true>;

    template
    class TarjanModelChecker<LTL::ResumingSuccessorGenerator, false>;

    template
    class TarjanModelChecker<LTL::SpoolingSuccessorGenerator, true>;

    template
    class TarjanModelChecker<LTL::SpoolingSuccessorGenerator, false>;
}
