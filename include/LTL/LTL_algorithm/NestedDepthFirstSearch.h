//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
#define VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H

#include <ptrie/ptrie_stable.h>

#include "ModelChecker.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/Structures/Queue.h"

using namespace PetriEngine;

namespace LTL {
    class NestedDepthFirstSearch : public ModelChecker {
    public:
        NestedDepthFirstSearch(const PetriNet &net, PetriEngine::PQL::Condition_ptr ptr)
                : ModelChecker(net, ptr), mark1(net, 0), mark2(net, 0) {}

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;

        PetriEngine::Structures::StateSet mark1;
        PetriEngine::Structures::StateSet mark2;
        Structures::ProductState *seed = nullptr;
        bool violation = false;

        void dfs(Structures::ProductState &state);

        void ndfs(Structures::ProductState &state);
    };
}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
