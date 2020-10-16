/*
 * File:   TarjanModelChecker.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/LTL_algorithm/TarjanModelChecker.h"

namespace LTL {


#ifdef PRINTF_DEBUG

    inline void _dump_state(const LTL::Structures::ProductState &state, int nplaces = -1) {
        if (nplaces == -1) nplaces = state.buchi_state_idx;
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (int i = 1; i <= nplaces; ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }

#endif

    bool LTL::TarjanModelChecker::isSatisfied() {
        {
            std::vector<State> initial_states;
            successorGenerator.makeInitialState(initial_states);
            for (auto &state : initial_states) {
                seen.add(state);
                push(state);
            }
        }
        State working = factory.newState();
        while (!dstack.empty() && !violation) {
            DStack &dtop = dstack.top();
            seen.decode(working, cstack[dtop.pos].stateid);
            if (!nexttrans(working, dtop)) {
                pop();
                continue;
            }
            auto res = seen.lookup(working);

            if (res.first) {
                auto p = chash[hash(res.second)];
                while (p != std::numeric_limits<idx_t>::max() && cstack[p].stateid != res.second) {
                    p = cstack[p].next;
                }
                if (p != std::numeric_limits<idx_t>::max()) {
                    update(p);
                    continue;
                }
            }
            if (!store.lookup(working).first) {
                push(working);
            }
        }
        return !violation;
    }

    void TarjanModelChecker::push(State &state) {
        const auto res = seen.lookup(state);
        assert(res.first);
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(res.second);
        cstack.emplace_back(ctop, res.second, chash[h]);
        chash[h] = ctop;
        dstack.push({ctop});
        if (successorGenerator.isAccepting(state)) {
            astack.push(ctop);
        }
    }

    void TarjanModelChecker::pop() {
        const size_t p = dstack.top().pos;
        dstack.pop();
        if (cstack[p].lowlink == p) {
            State tmp = factory.newState();
            while (cstack.size() > p) {
                auto h = hash(cstack.back().stateid);
                seen.decode(tmp, cstack.back().stateid);
                store.add(tmp);
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

    void TarjanModelChecker::update(idx_t to) {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
        }
    }

    bool TarjanModelChecker::nexttrans(State &state, TarjanModelChecker::DStack &delem) {
        if (!delem.neighbors) {
            delem.neighbors = std::vector<idx_t>();
            delem.nexttrans = 0;
            State tmp = factory.newState();
            seen.decode(tmp, cstack[delem.pos].stateid);
            successorGenerator.prepare(&tmp);
            while (successorGenerator.next(tmp)) {
                auto res = seen.lookup(tmp);
                if (res.first) {
                    delem.neighbors->push_back(res.second);
                    continue;
                }
                res = seen.add(tmp);
                assert(res.first);
                delem.neighbors->push_back(res.second);
            }
        }
        if (delem.nexttrans == delem.neighbors->size()) {
            return false;
        } else {
            seen.decode(state, (*delem.neighbors)[delem.nexttrans]);
            delem.nexttrans++;
            return true;
        }
    }

}