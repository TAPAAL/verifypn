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
    bool StubbornTarjanModelChecker::isSatisfied() {
        is_weak = successorGenerator->is_weak() && shortcircuitweak;
        std::vector<State> initial_states;
        successorGenerator->makeInitialState(initial_states);
        State working = factory.newState();
        State parent = factory.newState();
        for (auto &state : initial_states) {
            const auto res = seen.insertProductState(state);
            if (res.first) {
                push(state);
            }
            while (!dstack.empty() && !violation) {
                DEntry &dtop = dstack.top();
                if (!nexttrans(working, parent, dtop)) {
                    pop();
                    continue;
                }
                const idx_t stateid = seen.insertProductState(working).second;

                // lookup successor in 'hash' table
                idx_t p = searchCStack(stateid);
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
        }
        return !violation;
    }

    inline void _dump_state(const LTL::Structures::ProductState &state) {
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (size_t i = 1; i < state.size(); ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }

    /**
     * Push a state to the various stacks.
     * @param state
     */
    void StubbornTarjanModelChecker::push(State &state) {
        const auto res = seen.insertProductState(state);
        const auto ctop = static_cast<idx_t>(cstack.size());
        const auto h = hash(res.second);
        cstack.emplace_back(ctop, res.second, chash[h]);
        chash[h] = ctop;
        dstack.push(DEntry{ctop});
        if (successorGenerator->isAccepting(state)) {
            astack.push(ctop);
        }
    }

    void StubbornTarjanModelChecker::pop() {
        const auto p = dstack.top().pos;
        dstack.pop();
        if (cstack[p].lowlink == p) {
            while (cstack.size() > p) {
                popCStack();
            }
        } else if (is_weak) {
            State state = factory.newState();
            seen.retrieveProductState(state, cstack[p].stateid);
            if (!successorGenerator->isAccepting(state)) {
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

    void StubbornTarjanModelChecker::popCStack() {
        auto h = hash(cstack.back().stateid);
        store.insert(cstack.back().stateid);
        chash[h] = cstack.back().next;
        cstack.pop_back();
    }

    void StubbornTarjanModelChecker::update(idx_t to) {
        const auto from = dstack.top().pos;
        if (cstack[to].lowlink <= cstack[from].lowlink) {
            // we have found a loop into earlier seen component cstack[to].lowlink.
            // if this earlier component was found before an accepting state,
            // we have found an accepting loop and thus a violation.
            violation = (!astack.empty() && to <= astack.top());
            cstack[from].lowlink = cstack[to].lowlink;
        }
    }

    bool StubbornTarjanModelChecker::nexttrans(State &state, State &parent, DEntry &delem) {
        if (delem.successors.empty()) {
            if (delem.expanded) {
                return false;
            }
            seen.retrieveProductState(parent, cstack[delem.pos].stateid);
            delem.expanded = true;
            light_deque<size_t> successors;
            successorGenerator->prepare(&parent);
            while (successorGenerator->next(state)) {
                ++stats.explored;
                auto res = seen.insertProductState(state);
                // TODO search for repeat marking instead of repeat Büchi state.
                if (searchCStack(res.second) != std::numeric_limits<idx_t>::max()) {
                    successorGenerator->generateAll();
                }
                if (res.first) {
                    successors.push_back(res.second);
                }
            }
            delem.successors = successors;
            if (!delem.successors.empty()) {
                seen.retrieveProductState(state, delem.successors.front());
                delem.successors.pop_front();
            }
            return true;
            idx_t lastgenerated = std::numeric_limits<idx_t>::max();
            while (true) {
                if (successorGenerator->next(state)) {
                    ++stats.explored;
                    if (lastgenerated != std::numeric_limits<idx_t>::max()) {
                        delem.successors.push_back(lastgenerated);
                    }
                    auto res = seen.insertProductState(state);
                    // don't explore previously visited states
                    if (res.first) lastgenerated = res.second;
                    auto p = searchCStack(res.second);
                    if (p != std::numeric_limits<idx_t>::max()) {
                        successorGenerator->generateAll();
                    }
                } else {
                    ++stats.expanded;
                    // return state of last transition instead of first, saving one state operation.
                    return true;
                }

            }
        } else {
            auto stateid = delem.successors.front();
            delem.successors.pop_front();
            seen.retrieveProductState(state, stateid);
            return true;
        }
    }
}
