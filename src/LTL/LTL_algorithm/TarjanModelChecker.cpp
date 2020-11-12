/*
 * File:   TarjanModelChecker.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/LTL_algorithm/TarjanModelChecker.h"

#undef PRINTF_DEBUG
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
        std::vector<State> initial_states;
        successorGenerator->makeInitialState(initial_states);
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            auto res = seen.add(state);
            if (res.first) {
                push(state);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                if (!nexttrans(working, parent, dtop)) {
                    pop();
                    continue;
                }
                idx_t stateid = seen.add(working).second;
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
                    State next = factory.newState();
                    std::cerr << "Printing trace: " << std::endl;
                    std::vector<DEntry> rev;
                    auto sz = dstack.size();
                    // dump stack to vector to allow iteration
                    for (size_t i = 0; i < sz; ++i) {
                        rev.push_back(dstack.top());
                        dstack.pop();
                    }
                    idx_t pos = 0;
                    // print dstack in-order. rev[0] is dstack.top(), so loop vector in reverse
                    for (int i = rev.size() - 1; i >= 0; --i) {
                        pos = rev[i].pos;
                        seen.decode(parent, cstack[pos].stateid);
                        _dump_state(parent);
                        cstack[pos].lowlink = std::numeric_limits<idx_t>::max();
                        if (i > 0) {
                            // print transition to next state
                            seen.decode(next, cstack[rev[i - 1].pos].stateid);
                            successorGenerator->prepare(&parent);
                            while (successorGenerator->next(working)) {
                                if (working == next) {
                                    std::cerr << net.transitionNames()[successorGenerator->last_transition()]
                                              << std::endl;
                                    break;
                                }
                            }
                        }
                    }

                    // follow lowsource until back in dstack
                    pos = cstack[pos].lowsource;
                    if (cstack[pos].lowlink != std::numeric_limits<idx_t>::max()) {
                        std::cerr << "Printing looping part\n";
                    }
                    while (cstack[pos].lowlink != std::numeric_limits<idx_t>::max()) {
                        seen.decode(parent, cstack[pos].stateid);
                        _dump_state(parent);
                        pos = cstack[pos].lowsource;
                    }
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
        const size_t p = dstack.top().pos;
        dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                auto h = hash(cstack.back().stateid);
                store.insert(cstack.back().stateid);
                chash[h] = cstack.back().next;
                cstack.pop_back();
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
    void TarjanModelChecker<SaveTrace>::update(idx_t to) {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
            if constexpr (SaveTrace) {
                cstack[from].lowsource = to;
            }
        }
    }

    template<bool SaveTrace>
    bool TarjanModelChecker<SaveTrace>::nexttrans(State &state, State &parent, TarjanModelChecker::DEntry &delem) {
        seen.decode(parent, cstack[delem.pos].stateid);
#ifdef PRINTF_DEBUG
        std::cerr << "loaded parent\n  ";
        _dump_state(parent);
#endif
        successorGenerator->prepare(&parent, delem.sucinfo);
        if (delem.sucinfo.has_prev_state()) {
            seen.decode(state, delem.sucinfo.last_state);
        }
        auto res = successorGenerator->next(state, delem.sucinfo);
#ifdef PRINTF_DEBUG
        if (res) {
            std::cerr << "going to state\n";
            _dump_state(state);
        }
#endif
        return res;
    }

    template
    class TarjanModelChecker<true>;

    template
    class TarjanModelChecker<false>;
}