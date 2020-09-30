//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"

namespace LTL {

    bool NestedDepthFirstSearch::isSatisfied() {
        State init;
        init.setMarking(net.makeInitialMarking());
        dfs(init);
        return !violation;
    }

    void NestedDepthFirstSearch::dfs(LTL::Structures::ProductState &state) {

        PQL::DistanceContext ctx{&net, state.marking()};
        std::stack<State *> call_stack;
        State working;
        State curState;
        successorGenerator->initial_state(working);
        successorGenerator->initial_state(curState);

        PetriEngine::Structures::StateSet states{net, 0};
        PetriEngine::Structures::DFSQueue todo{&states};
        if (auto r = states.add(state); !r.first) {
            throw std::runtime_error{"LTL: Could not add initial state to state set"};
        } else {
            todo.push(r.second, ctx, formula);
        }
        while (todo.top(curState)) {

            if (!call_stack.empty() && &curState == call_stack.top()) {
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation)
                        return;
                }
                todo.pop(curState);
                call_stack.pop();
            } else {
                call_stack.push(&curState);
                if (!mark1.add(curState).first) {
                    continue;
                }
                successorGenerator->prepare(&curState);
                while (successorGenerator->next(working)) {
                    auto r = states.add(working);
                    todo.push(r.second, ctx, formula);
                }
            }
        }
    }

    void NestedDepthFirstSearch::ndfs(Structures::ProductState &state) {
        PetriEngine::Structures::StateSet states{net, 0};
        PetriEngine::Structures::DFSQueue todo{&states};
        State working;
        State curState;

        successorGenerator->initial_state(working);
        successorGenerator->initial_state(curState);

        auto r = states.add(state);
        assert(r.first);
        PQL::DistanceContext ctx{&net, state.marking()};

        todo.push(r.second, ctx, formula);
        while (todo.pop(curState)) {

            if (!mark2.add(curState).first) {
                continue;
            }

            successorGenerator->prepare(&curState);
            while (successorGenerator->next(working)) {
                if (working == *seed) {
                    violation = true;
                    return;
                }
                r = states.add(working);
                if (r.first) {
                    todo.push(r.second, ctx, formula);
                }
            }
        }
    }
}