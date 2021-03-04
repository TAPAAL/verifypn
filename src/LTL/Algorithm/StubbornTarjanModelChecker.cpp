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

#include "LTL/Algorithm/StubbornTarjanModelChecker.h"

namespace LTL {
    inline void _dump_state(const LTL::Structures::ProductState &state)
    {
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (size_t i = 1; i < state.size(); ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << "\tbuchi " << state.getBuchiState();
        std::cerr << std::endl;
    }

    template<typename W, typename G>
    bool StubbornTarjanModelChecker<W, G>::isSatisfied()
    {
        this->is_weak = this->successorGenerator->is_weak() && this->shortcircuitweak;
        std::vector<State> initial_states;
        this->successorGenerator->makeInitialState(initial_states);
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            const auto res = seen.add(state);
            if (res.first) {
                push(state, res.second);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                if (!nexttrans(working, parent, dtop)) {
                    pop();
                    continue;
                }
                const idx_t stateid = seen.add(working).second;

                // lookup successor in 'hash' table
                idx_t p = searchCStack(stateid);
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
    template<typename W, typename G>
    void StubbornTarjanModelChecker<W, G>::push(State &state, size_t stateid)
    {
        //const auto res = seen.add(state);
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(stateid);
        cstack.emplace_back(ctop, stateid, chash[h]);
        chash[h] = ctop;
        dstack.push(DEntry{ctop});
        if (this->successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
    }

    template<typename W, typename G>
    void StubbornTarjanModelChecker<W, G>::pop()
    {
        const auto p = dstack.top().pos;
        dstack.pop();
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
        if (!dstack.empty()) {
            update(p);
        }
    }

    template<typename W, typename G>
    void StubbornTarjanModelChecker<W, G>::popCStack()
    {
        auto h = hash(cstack.back().stateid);
        store.insert(cstack.back().stateid);
        chash[h] = cstack.back().next;
        cstack.pop_back();
    }

    template<typename W, typename G>
    void StubbornTarjanModelChecker<W, G>::update(idx_t to)
    {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            if constexpr (SaveTrace) {
                cstack[from].lowsource = to;
                if (violation) {
                    loopstate = cstack[to].stateid;
                    looptrans = _lasttid;
                }
            }
            cstack[from].lowlink = cstack[to].lowlink;
        }
    }

//#define PRINTF_NEXTRANS

    template<typename W, typename G>
    bool StubbornTarjanModelChecker<W, G>::nexttrans(State &state, State &parent, DEntry &delem)
    {
        if (delem.successors.empty()) {
            if (delem.expanded) {
                return false;
            }
            seen.decode(parent, cstack[delem.pos].stateid);
            delem.expanded = true;
            light_deque<DSucc> successors;
            this->successorGenerator->prepare(&parent);
            while (this->successorGenerator->next(state)) {
                ++this->stats.explored;
                auto[_new, stateid] = seen.add(state);
#ifdef PRINTF_NEXTRANS
                const PetriEngine::PetriNet &net = this->successorGenerator->getNet();
                std::cerr << cstack[delem.pos].stateid << "-" << net.transitionNames()[this->successorGenerator->fired()] << "> " << stateid << " Buchi: " << state.getBuchiState() << std::endl;
#endif
                // from textbook LTL / bit product state
                /*auto markingId = seen.getMarkingId(stateid);
                auto p = chash[hash(stateid)];
                while (p != numeric_limits<idx_t>::max() && seen.getMarkingId(cstack[p].stateid) != markingId) {
                    p = cstack[p].next;
                }
                if (p != std::numeric_limits<idx_t>::max()) {
                    this->successorGenerator->generateAll();
                }*/
                //if (_new) {
                if constexpr (SaveTrace) {
                    auto tid = this->successorGenerator->fired();
                    successors.push_back(DSucc{stateid, tid});
                    //if (_new)
                    seen.setHistory(stateid, tid);
                } else {
                    successors.push_back(DSucc{stateid});
                }
            }
            delem.successors = successors;
            if (!delem.successors.empty()) {
                seen.decode(state, delem.successors.front().id);
                if constexpr (SaveTrace) {
                    _lasttid = delem.successors.front().trans;

                }
                delem.successors.pop_front();
                return true;
            }
            else return false;
            /*idx_t lastgenerated = std::numeric_limits<idx_t>::max();
            while (true) {
                if (this->successorGenerator->next(state)) {
                    ++this->stats.explored;
                    if (lastgenerated != std::numeric_limits<idx_t>::max()) {
                        delem.successors.push_back(lastgenerated);
                    }
                    auto res = seen.add(state);
                    // don't explore previously visited states
                    if (res.first) lastgenerated = res.second;
                    auto p = searchCStack(res.second);
                    if (p != std::numeric_limits<idx_t>::max()) {
                        this->successorGenerator->generateAll();
                    }
                } else {
                    ++this->stats.expanded;
                    // return state of last transition instead of first, saving one state operation.
                    return true;
                }

            }*/
        } else {
            auto stateid = delem.successors.front();
            if constexpr (SaveTrace) {
                _lasttid = delem.successors.front().trans;
            }
            delem.successors.pop_front();
            seen.decode(state, stateid.id);
            return true;
        }
    }

    template<typename W, typename G>
    std::ostream &StubbornTarjanModelChecker<W, G>::printTransition(size_t transition, uint indent, std::ostream &os)
    {
        if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
            os << std::string(indent, '\t') << "<deadlock/>";
            return os;
        }
        std::string tname = seen.net().transitionNames()[transition];
        os << std::string(indent, '\t') << "<transition id=\"" << tname << "\" index=\"" << transition << "\"/>";
        return os;
    }

    template<typename W, typename G>
    void StubbornTarjanModelChecker<W, G>::printTrace(std::stack<DEntry> &&revdstack, std::ostream &os)
    {
        if constexpr (!SaveTrace) {
            return;
        } else {
            revdstack.pop();
            os << "Trace:\n"
                  "<trace>\n";
            auto indent = 1;
            long p;
            // print (reversed) dstack
            while (!revdstack.empty()) {
                p = revdstack.top().pos;
                auto stateid = cstack[p].stateid;
                auto[parent, tid] = seen.getHistory(stateid);
                if (stateid == loopstate) {
                    ++indent;
                    os << "\t<loop>\n";
                }
                printTransition(tid, indent, os) << '\n';
                cstack[p].lowlink = std::numeric_limits<idx_t>::max();
                revdstack.pop();
            }
            // follow previously found back edges via lowsource until back in revdstack.
            p = cstack[p].lowsource;
            while (cstack[p].lowlink != std::numeric_limits<idx_t>::max()) {
                auto[parent, tid] = seen.getHistory(cstack[p].stateid);
                printTransition(tid, indent, os);
                p = cstack[p].lowsource;
            }
            printTransition(_lasttid, indent, os) << '\n';

            os << "\t</loop>\n</trace>" << std::endl;
        }
    }

    template
    class StubbornTarjanModelChecker<PetriEngine::ReducingSuccessorGenerator, PetriEngine::Structures::StateSet>;

    template
    class StubbornTarjanModelChecker<PetriEngine::ReducingSuccessorGenerator, PetriEngine::Structures::TracableStateSet>;

    template
    class StubbornTarjanModelChecker<PetriEngine::SuccessorGenerator, PetriEngine::Structures::StateSet>;

    template
    class StubbornTarjanModelChecker<PetriEngine::SuccessorGenerator, PetriEngine::Structures::TracableStateSet>;

    template
    class StubbornTarjanModelChecker<LTL::ReducingSuccessorGenerator, PetriEngine::Structures::StateSet>;

    template
    class StubbornTarjanModelChecker<LTL::ReducingSuccessorGenerator, PetriEngine::Structures::TracableStateSet>;
}
