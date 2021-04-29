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

#ifndef VERIFYPN_HEURISTIC_H
#define VERIFYPN_HEURISTIC_H

#include "PetriEngine/PetriNet.h"
#include "LTL/Structures/ProductState.h"
#include <iostream>

namespace LTL {
    class Heuristic {
    public:
        virtual void prepare(const LTL::Structures::ProductState &state) {}

        virtual uint32_t eval(const LTL::Structures::ProductState &state, uint32_t tid) = 0;

        /**
         * Does the heuristic provide a prioritisation from this state.
         * @return True if a heuristic can be calculated from this state.
         */
        virtual bool has_heuristic(const LTL::Structures::ProductState &)
        {
            return true;
        }

        virtual void push(uint32_t tid) {};

        virtual void pop(uint32_t tid) {};

        virtual ~Heuristic() = default;

        virtual std::ostream &output(std::ostream &os) = 0;
    };
}

#endif //VERIFYPN_HEURISTIC_H
