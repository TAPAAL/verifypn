/*
 * File:   TarjanModelChecker.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/LTL_algorithm/TarjanModelChecker.h"

namespace LTL {

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
            else {
                auto res = seen.lookup(working);
                if (res.first && hash_search(res.second) != std::numeric_limits<idx_t>::max()) {
                    update(res.second);
                    continue;
                }
                if (!store.lookup(working).first) {
                    push(working);
                }
            }
        }
        return !violation;
    }

    void TarjanModelChecker::push(State &state) {
        const auto res = seen.lookup(state);
        assert(res.first);
        const auto ctop = static_cast<idx_t>(cstack.size());
        cstack.push_back({ctop, res.second});
        cstack.back().next = hash_search(res.second);
        hash_search(res.second) = ctop;
        dstack.push({ctop, std::numeric_limits<uint32_t>::max()});
        if (successorGenerator.isAccepting(state)) {
            astack.push(ctop);
        }
    }

    void TarjanModelChecker::pop() {
        const auto p = dstack.top().pos; dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                State tmp = factory.newState();
                seen.decode(tmp, cstack.back().stateid);
                store.add(tmp);
                hash_search(cstack.back().stateid) = cstack.back().next;
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
                    delem.neighbors->push_back(res.second); continue;
                }
                res = seen.add(tmp);
                assert(res.first);
                delem.neighbors->push_back(res.second);
            }
        }
        if (delem.nexttrans == delem.neighbors->size()) {
            return false;
        }
        else {
            seen.decode(state, (*delem.neighbors)[delem.nexttrans]);
            delem.nexttrans++;
            return true;
        }
    }
}