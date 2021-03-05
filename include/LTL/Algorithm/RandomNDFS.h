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

#ifndef VERIFYPN_RANDOMNDFS_H
#define VERIFYPN_RANDOMNDFS_H

#include "ModelChecker.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/Structures/Queue.h"
#include "LTL/Structures/ProductStateFactory.h"

#include <ptrie/ptrie_stable.h>

using namespace PetriEngine;

namespace LTL {
    class RandomNDFS : public ModelChecker {
    public:
        RandomNDFS(const PetriNet &net, PetriEngine::PQL::Condition_ptr ptr)
                : ModelChecker(net, ptr, false), factory{net, successorGenerator->initial_buchi_state()},
                  mark1(net, 0, (int) net.numberOfPlaces() + 1), mark2(net, 0, (int) net.numberOfPlaces() + 1) {}

        bool isSatisfied() override;

        void printStats(ostream &os) override;

    private:
        using State = LTL::Structures::ProductState;

        Structures::ProductStateFactory factory;
        PetriEngine::Structures::StateSet mark1;
        PetriEngine::Structures::StateSet mark2;


        State *seed;
        bool violation = false;

        void dfs();

        void ndfs(State &state);
    };
}

#endif //VERIFYPN_RANDOMNDFS_H
