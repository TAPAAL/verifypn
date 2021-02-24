/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_REDUCINGSUCCESSORGENERATOR_H
#define VERIFYPN_REDUCINGSUCCESSORGENERATOR_H

#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "LTL/Stubborn/AutomatonStubbornSet.h"

#include <utility>

namespace LTL {
    class ReducingSuccessorGenerator : public PetriEngine::ReducingSuccessorGenerator {
    public:
        ReducingSuccessorGenerator(const PetriEngine::PetriNet &net,
                                   std::shared_ptr<PetriEngine::StubbornSet> stubbornSet)
                : PetriEngine::ReducingSuccessorGenerator(net, std::move(stubbornSet)) {}

        void prepare(const PetriEngine::Structures::State *state, const LTL::GuardInfo &info)
        {
            _current = 0;
            _parent = state;
            (dynamic_pointer_cast<LTL::AutomatonStubbornSet>(_stubSet))->prepare(state, info);
        }

    };
}

#endif //VERIFYPN_REDUCINGSUCCESSORGENERATOR_H
