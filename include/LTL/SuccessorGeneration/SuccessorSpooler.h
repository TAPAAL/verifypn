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

#ifndef VERIFYPN_SUCCESSORSPOOLER_H
#define VERIFYPN_SUCCESSORSPOOLER_H

#include "LTL/Structures/ProductState.h"

namespace LTL {
    class SuccessorSpooler {
    public:
        virtual bool prepare(const LTL::Structures::ProductState *) = 0;

        virtual uint32_t next() = 0;

        virtual ~SuccessorSpooler() = default;

        virtual void reset()
        {

        }

        virtual void generateAll()
        {

        }
        static constexpr uint32_t NoTransition = std::numeric_limits<uint32_t>::max();
    };
}

#endif //VERIFYPN_SUCCESSORSPOOLER_H
