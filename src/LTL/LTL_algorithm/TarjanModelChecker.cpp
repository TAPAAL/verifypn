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
            successorGenerator->makeInitialState(initial_states);
            for (auto &state : initial_states) {
                seen.add(state);
                push(state);
            }
        }
        State working = factory.newState();
        State parent = factory.newState();
        while (!dstack.empty() && !violation) {
            DEntry &dtop = dstack.top();
            if (!nexttrans(working, parent, dtop)) {
                pop();
                continue;
            }
            idx_t stateid = seen.add(working).second;

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
        return !violation;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    void TarjanModelChecker::push(State &state) {
        const auto res = seen.add(state);
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(res.second);
        cstack.emplace_back(ctop, res.second, chash[h]);
        chash[h] = ctop;
        dstack.push(DEntry{ctop, initial_suc_info});
        if (successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
    }

    void TarjanModelChecker::pop() {
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

    void TarjanModelChecker::update(idx_t to) {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop is into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
        }
    }

    bool TarjanModelChecker::nexttrans(State &state, State &parent, TarjanModelChecker::DEntry &delem) {
        seen.decode(parent, cstack[delem.pos].stateid);
#ifdef PRINTF_DEBUG
        std::cerr << "loaded parent\n  ";
        _dump_state(parent);
#endif
        successorGenerator->prepare(&parent, delem.sucinfo);
        auto res = successorGenerator->next(state, delem.sucinfo);
#ifdef PRINTF_DEBUG
        if (res) {
            std::cerr << "going to state\n";
            _dump_state(state);
        }
#endif
        return res;
    }
}