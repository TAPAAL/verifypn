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

    inline void _dump_state(const LTL::Structures::ProductState &state) {
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (size_t i = 1; i < state.size(); ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }

    template<bool SaveTrace>
    bool TarjanModelChecker<SaveTrace>::isSatisfied() {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        std::vector<State> initial_states;
        successorGenerator->makeInitialState(initial_states);
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            const auto res = seen.add(state);
            if (res.first) {
                push(state);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                if (!nexttrans(working, parent, dtop)) {
                    pop();
                    continue;
                }
                auto[isnew, stateid] = seen.add(working);
                if constexpr (SaveTrace) {
                    if (isnew) {
                        seen.setHistory(stateid, successorGenerator->fired());
                    }
                }
                dtop.sucinfo.last_state = stateid;

                // lookup successor in 'hash' table
                auto p = chash[hash(stateid)];
                while (p != std::numeric_limits<idx_t>::max() && cstack[p].stateid != stateid) {
                    p = cstack[p].next;
                }
                if (p != std::numeric_limits<idx_t>::max()) {
                    // we found the successor, i.e. there's a loop!
                    // now update lowlinks and check whether the loop contains an accepting state
                    update(p);
                    continue;
                }
                if (store.find(stateid) == std::end(store)) {
                    push(working);
                }
            }
            if constexpr (SaveTrace) {
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
    template<bool SaveTrace>
    void TarjanModelChecker<SaveTrace>::push(State &state) {
        const auto res = seen.add(state);
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(res.second);
        cstack.emplace_back(ctop, res.second, chash[h]);
        if constexpr (SaveTrace) {
            cstack.back().lowsource = ctop;
        }
        chash[h] = ctop;
        dstack.push(DEntry{ctop, initial_suc_info});
        if (successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
    }

    template<bool SaveTrace>
    void TarjanModelChecker<SaveTrace>::pop() {
        const auto p = dstack.top().pos;
        dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                popCStack();
            }
        } else if (is_weak) {
            State state = factory.newState();
            seen.decode(state, cstack[p].stateid);
            if(!successorGenerator->isAccepting(state)){
                popCStack();
            }
        }
        if (!astack.empty() && p == astack.top()) {
            astack.pop();
        }
        if (!dstack.empty()) {
            update(p);
        }
    }

    template<bool SaveTrace>
    void TarjanModelChecker<SaveTrace>::popCStack(){
        auto h = hash(cstack.back().stateid);
        store.insert(cstack.back().stateid);
        chash[h] = cstack.back().next;
        cstack.pop_back();
    }

    template<bool SaveTrace>
    void TarjanModelChecker<SaveTrace>::update(idx_t to) {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
            if constexpr (SaveTrace) {
                loopstate = cstack[to].stateid;
                looptrans = successorGenerator->fired();
                cstack[from].lowsource = cstack[to].lowlink;

            }
        }
    }

    template<bool SaveTrace>
    bool TarjanModelChecker<SaveTrace>::nexttrans(State &state, State &parent, TarjanModelChecker::DEntry &delem) {
        seen.decode(parent, cstack[delem.pos].stateid);
        successorGenerator->prepare(&parent, delem.sucinfo);
        if (delem.sucinfo.has_prev_state()) {
            seen.decode(state, delem.sucinfo.last_state);
        }
        auto res = successorGenerator->next(state, delem.sucinfo);
        return res;
    }

    template<bool SaveTrace>
    std::ostream &TarjanModelChecker<SaveTrace>::printTransition(size_t transition, uint indent, std::ostream &os)
    {
        if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
            os << std::string(indent, '\t') << "<deadlock/>";
            return os;
        }
        std::string tname = seen.net().transitionNames()[transition];
        os << std::string(indent, '\t') << "<transition id=\"" << tname << "\" index=\"" << transition << "\"/>";
        return os;
    }

    template<bool SaveTrace>
    void TarjanModelChecker<SaveTrace>::printTrace(std::stack<DEntry> &&dstack, std::ostream &os)
    {
        if constexpr (!SaveTrace) {
            return;
        } else {
            dstack.pop();
            os << "Trace:\n"
                  "<trace>\n";
            auto indent = 1;
            long p;
            // print (reverted) dstack
            while (!dstack.empty()) {
                p = dstack.top().pos;
                auto stateid = cstack[p].stateid;
                auto[parent, tid] = seen.getHistory(stateid);
                printTransition(tid, indent, os) << '\n';
                cstack[p].lowlink = std::numeric_limits<idx_t>::max();
                dstack.pop();
            }
            ++indent;
            os << "\t<loop>\n";
            // follow previously found back edges via lowsource until back in dstack.
            p = cstack[p].lowsource;
            while (cstack[p].lowlink != std::numeric_limits<idx_t>::max()) {
                auto[parent, tid] = seen.getHistory(cstack[p].stateid);
                printTransition(tid, indent, os);
                p = cstack[p].lowsource;
            }
            printTransition(looptrans, indent, os) << '\n';

            os << "\t</loop>\n</trace>" << std::endl;
        }
    }



    template
    class TarjanModelChecker<true>;

    template
    class TarjanModelChecker<false>;
}
