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

#ifndef VERIFYPN_GUARDINFO_H
#define VERIFYPN_GUARDINFO_H

#include "PetriEngine/PQL/PQL.h"
#include <vector>

namespace LTL {
    struct GuardInfo {
        GuardInfo(size_t buchiState, bool isAccepting) : buchi_state(buchiState), is_accepting(isAccepting) {}

        int buchi_state;
        PetriEngine::PQL::Condition_ptr retarding;
        std::vector<PetriEngine::PQL::Condition_ptr> progressing;
        bool is_accepting;
    };
}

#endif //VERIFYPN_GUARDINFO_H
