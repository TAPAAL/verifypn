//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
#define VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H

#include "ModelChecker.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/Structures/Queue.h"
#include "LTL/Structures/ProductStateFactory.h"

#include <ptrie/ptrie_stable.h>

using namespace PetriEngine;

namespace LTL {
    class NestedDepthFirstSearch : public ModelChecker {
    public:
        NestedDepthFirstSearch(const PetriNet &net, PetriEngine::PQL::Condition_ptr ptr)
                : ModelChecker(net, ptr), factory{net, successorGenerator->initial_buchi_state()},
                  mark1(net, 0, (int)net.numberOfPlaces() + 1), mark2(net, 0, (int)net.numberOfPlaces() + 1) {}

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;

        Structures::ProductStateFactory factory;
        PetriEngine::Structures::StateSet mark1;
        PetriEngine::Structures::StateSet mark2;

        struct StackEntry {
            size_t id;
            successor_info sucinfo;
        };

        State *seed;
        bool violation = false;

        void dfs();

        void ndfs(State &state);
    };
}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
