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

#ifndef COLORED_MODELCHECKER_H
#define COLORED_MODELCHECKER_H

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include <LTL/SuccessorGeneration/Heuristic.h>

namespace ColoredLTL {
    class ColoredModelChecker {
    public:
        ColoredModelChecker(const PetriEngine::ExplicitColored::ColoredPetriNet& net,
                const PetriEngine::PQL::Condition_ptr &condition)
        : _net(net), _formula(condition){
        }

        virtual bool check() = 0;

        virtual ~ColoredModelChecker() = default;


    protected:
        const PetriEngine::ExplicitColored::ColoredPetriNet& _net;
        PetriEngine::PQL::Condition_ptr _formula;
    };
}

#endif //COLORED_MODELCHECKER_H
