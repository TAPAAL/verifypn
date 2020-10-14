/*
 * File:   TarjanModelChecker.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/LTL_algorithm/TarjanModelChecker.h"
#include <stack>

namespace LTL {

    bool LTL::TarjanModelChecker::isSatisfied() {
        State working = factory.makeInitialState();
        push(working);
        while (!dstack.empty() && !violation) {
            auto dtop = dstack.top();
            seen.decode(working, cstack[dtop.pos].stateid);
            if (!successorGenerator.next(working, dtop.lasttrans)) {
                pop();
                continue;
            }
            else {
                if (auto res = seen.lookup(working); res.first) {
                    update(res.second);
                    continue;
                }
                if (!store.lookup(working).first) {
                    push(working);
                }
            }
        }
        return violation;
    }

    void TarjanModelChecker::push(State &state) {
        const auto res = seen.add(state);
        assert(res.first);
        const auto ctop = cstack.size();
        cstack.push_back({res.second, ctop});
        dstack.push({ctop, std::numeric_limits<idx_t>::max()});
        if (successorGenerator.isAccepting(state)) {
            astack.push(ctop);
        }
    }

    void TarjanModelChecker::pop() {
        const auto p = dstack.top().pos; dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() - 1 > p) {
                State tmp = factory.makeInitialState();
                seen.decode(tmp, cstack.back().stateid);
                store.add(tmp);
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
}