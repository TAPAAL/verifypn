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
#include <unordered_set>

using namespace PetriEngine;

namespace LTL {
    class NestedDepthFirstSearch : public ModelChecker {
    public:
        NestedDepthFirstSearch(const PetriNet &net, PetriEngine::PQL::Condition_ptr ptr, const bool shortcircuitweak)
                : ModelChecker(net, ptr), factory{net, successorGenerator->initial_buchi_state()},
                states(net, 0, (int)net.numberOfPlaces() + 1), shortcircuitweak(shortcircuitweak) {}

        bool isSatisfied() override;

    private:
        using State = LTL::Structures::ProductState;

        Structures::ProductStateFactory factory;
        PetriEngine::Structures::StateSet states;
        std::set<size_t> mark1;
        std::set<size_t> mark2;

        struct StackEntry {
            size_t id;
            successor_info sucinfo;
        };

        State *seed;
        bool violation = false;
        bool is_weak = false;
        const bool shortcircuitweak;

        void dfs();

        void ndfs(State &state);
    };
}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
