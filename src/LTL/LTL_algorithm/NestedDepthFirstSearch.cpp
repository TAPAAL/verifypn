//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/NestedDepthFirstSearch.h"

namespace LTL {

    bool NestedDepthFirstSearch::isSatisfied() {
        return false; //TODO implement
    }

    void NestedDepthFirstSearch::dfs(LTL::Structures::ProductState &state) {
        std::stack<LTL::Structures::ProductState> todo;
        std::stack<LTL::Structures::ProductState*> call_stack;
        LTL::Structures::ProductState working;
        todo.push(state);
        while (!todo.empty()) {
            LTL::Structures::ProductState curState = todo.top();

            if (&curState == call_stack.top()) {
                if (successorGenerator.isAccepting(curState)){
                    seed = &curState;
                    ndfs(curState);
                    if (violation)
                        return;
                }
                todo.pop();
                call_stack.pop();
            } else {
                call_stack.push(&curState);
                if (!mark1.add(curState).first) {
                    continue;
                }
                successorGenerator.prepare(&curState);
                while (successorGenerator.next(working)) {
                    todo.push(working);
                }
            }
        }
    }

    void NestedDepthFirstSearch::ndfs(Structures::ProductState& state) {
        std::stack<Structures::ProductState> todo;
        Structures::ProductState working;
        todo.push(state);
        while (!todo.empty()) {
            Structures::ProductState curState = todo.top();
            todo.pop();

            if (!mark2.add(curState).first) {
                continue;
            }

            successorGenerator.prepare(&curState);
            while (successorGenerator.next(working)) {
                if (working == *seed) {
                    violation = true;
                    return;
                }
                todo.push(working);
            }
        }
    }
}