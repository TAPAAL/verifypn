//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"

namespace LTL {

#ifdef PRINTF_DEBUG
    inline void dump_state(const LTL::Structures::ProductState &state, int nplaces=-1) {
        if (nplaces == -1) nplaces = state.buchi_state_idx;
        std::cerr << "marking: ";
        std::cerr << state.marking()[0];
        for (int i = 1; i <= nplaces; ++i) {
            std::cerr << ", " << state.marking()[i];
        }
        std::cerr << std::endl;
    }
#endif

    bool NestedDepthFirstSearch::isSatisfied() {
        is_weak = successorGenerator->is_weak();
        dfs();
#ifdef _PRINTF_DEBUG
        std::cout << "discovered " << _discovered << " states." << std::endl;
        std::cout << "mark1 size: " << mark1.size() << "\tmark2 size: " << mark2.size() << std::endl;
#endif
        return !violation;
    }

    void NestedDepthFirstSearch::dfs() {
        std::stack<size_t> call_stack;
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        {
            std::vector<State> initial_states;
            successorGenerator->makeInitialState(initial_states);
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(StackEntry{res.second, initial_suc_info});
                _discovered++;
            }
        }

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!successorGenerator->next(working, top.sucinfo)) {
                // no successor
                todo.pop();
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation) return;
                }
            } else {
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark1.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }
            }
        }
    }


    void NestedDepthFirstSearch::ndfs(State &state) {
        std::stack<StackEntry> todo;

        State working = factory.newState();
        State curState = factory.newState();

        todo.push(StackEntry{states.add(state).second, initial_suc_info});

        while (!todo.empty()) {
            auto &top = todo.top();
            states.decode(curState, top.id);
            successorGenerator->prepare(&curState, top.sucinfo);
            if (top.sucinfo.has_prev_state()) {
                states.decode(working, top.sucinfo.last_state);
            }
            if (!successorGenerator->next(working, top.sucinfo)) {
                todo.pop();
            } else {
                if (shortcircuitweak && is_weak && !successorGenerator->isAccepting(working)) {
                    continue;
                }
                if (working == *seed) {
#ifdef _PRINTF_DEBUG
                    std::cerr << "seed:\n  "; dump_state(*seed);
                    std::cerr << "working:\n  "; dump_state(working);
#endif
                    violation = true;
                    return;
                }
                auto[_, stateid] = states.add(working);
                auto[it, is_new] = mark2.insert(stateid);
                top.sucinfo.last_state = stateid;
                if (is_new) {
                    _discovered++;
                    todo.push(StackEntry{stateid, initial_suc_info});
                }

            }
        }
    }
}