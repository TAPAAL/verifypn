/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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
                : ModelChecker(net, ptr, shortcircuitweak), factory{net, successorGenerator->initial_buchi_state()},
                states(net, 0, (int)net.numberOfPlaces() + 1) {}

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

        void dfs();

        void ndfs(State &state);
    };
}

#endif //VERIFYPN_NESTEDDEPTHFIRSTSEARCH_H
