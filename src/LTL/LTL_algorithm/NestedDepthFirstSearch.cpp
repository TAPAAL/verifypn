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
        dfs();
        return !violation;
    }

    void NestedDepthFirstSearch::dfs() {
        std::stack<size_t> call_stack;

        PetriEngine::Structures::StateSet states{net,0, (int)net.numberOfPlaces() + 1};
        PetriEngine::Structures::DFSQueue todo{&states};

        State working = factory.newState();
        PQL::DistanceContext ctx{&net, working.marking()};
        State curState = factory.newState();
        {
            std::vector<State> initial_states;
            successorGenerator->makeInitialState(initial_states);
            for (auto &state : initial_states) {
                auto res = states.add(state);
                assert(res.first);
                todo.push(res.second, ctx, formula);
            }
        }

        while (todo.top(curState)) {

            if (!call_stack.empty() && states.lookup(curState).second == call_stack.top()) {
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation)
                        return;
                }
                todo.pop(curState);
                call_stack.pop();
            } else {
                call_stack.push(states.lookup(curState).second);
                if (!mark1.add(curState).first) {
                    continue;
                }
                successorGenerator->prepare(&curState);
#ifdef PRINTF_DEBUG
                std::cerr << "curState:\n";
                dump_state(curState);
#endif
                while (successorGenerator->next(working)) {
#ifdef PRINTF_DEBUG
                    std::cerr << "working:\n";
                    dump_state(working);
#endif
                    auto r = states.add(working);
                    todo.push(r.second, ctx, formula);
                }
            }
        }
    }


    void NestedDepthFirstSearch::ndfs(State& state) {
        PetriEngine::Structures::StateSet states{net, 0, (int)net.numberOfPlaces() + 1};
        PetriEngine::Structures::DFSQueue todo{&states};

        State working = factory.newState();
        State curState = factory.newState();

        PQL::DistanceContext ctx{&net, state.marking()};

        todo.push(states.add(state).second, ctx, formula);

        while (todo.pop(curState)) {
            if (!mark2.add(curState).first) {
                continue;
            }

            successorGenerator->prepare(&curState);
#ifdef PRINTF_DEBUG
            std::cerr << "curState:\n";
            dump_state(curState);
#endif
            while (successorGenerator->next(working)) {
#ifdef PRINTF_DEBUG
                std::cerr << "working:\n";
                dump_state(working);
#endif
                if (working == *seed) {
#ifdef PRINTF_DEBUG
                    std::cerr << "seed:\n  "; dump_state(*seed);
                    std::cerr << "working:\n  "; dump_state(working);
#endif
                    violation = true;
                    return;
                }
                auto r = states.add(working);
                todo.push(r.second, ctx, formula);
            }
        }

    }
}