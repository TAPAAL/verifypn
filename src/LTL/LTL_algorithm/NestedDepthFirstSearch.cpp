//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"

namespace LTL {

    bool NestedDepthFirstSearch::isSatisfied() {
        dfs();
        return !violation;
    }

    void NestedDepthFirstSearch::dfs() {
        std::stack<size_t> call_stack;

        PetriEngine::Structures::StateSet states{net,0};
        PetriEngine::Structures::DFSQueue todo{&states};

        State working = factory.makeInitialState();
        State curState = factory.makeInitialState();
        PQL::DistanceContext ctx{&net, working.marking()};

        auto res = states.add(working);
        assert(res.first);
        todo.push(res.second, ctx, formula);

        while (todo.top(curState)) {

            if (!call_stack.empty() && states.add(curState).second == call_stack.top()) {
                if (successorGenerator->isAccepting(curState)) {
                    seed = &curState;
                    ndfs(curState);
                    if (violation)
                        return;
                }
                todo.pop(curState);
                call_stack.pop();
            } else {
                call_stack.push(states.add(curState).second);
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


    void NestedDepthFirstSearch::ndfs(State& state) {
        PetriEngine::Structures::StateSet states{net, 0};
        PetriEngine::Structures::DFSQueue todo{&states};

        State working = factory.makeInitialState();
        State curState = factory.makeInitialState();

        PQL::DistanceContext ctx{&net, state.marking()};

        todo.push(states.add(state).second, ctx, formula);

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
                todo.push(states.add(working).second, ctx, formula);
            }
        }
    }
}