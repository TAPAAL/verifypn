/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLOREDNETSTRUCTURES_H
#define COLOREDNETSTRUCTURES_H


#include "Colors.h"
#include "Expressions.h"
#include "Multiset.h"

#include "utils/structures/shared_string.h"

#include <vector>
#include <set>
#include <cassert>

namespace PetriEngine {
    namespace Colored {

        struct Arc {
            uint32_t place;
            uint32_t transition;
            ArcExpression_ptr expr;
            bool input;
            uint32_t inhib_weight; // inhibitor arc if >0

            bool operator == (const Arc& other) const
            {
                return place == other.place && transition == other.transition && input == other.input && inhib_weight == other.inhib_weight && (inhib_weight > 0 || to_string(*expr) == to_string(*other.expr));
            }
        };

        [[maybe_unused]]
        struct {
            bool operator()(const Arc &a, const Arc &b) const
            {
                return a.place < b.place;
            }
        } ArcLessThanByPlace;

        struct Transition {
            shared_const_string name;
            GuardExpression_ptr guard;
            int32_t _player;
            double _x = 0, _y = 0;
            std::vector<Arc> input_arcs;
            std::vector<Arc> output_arcs;
            uint32_t inhibited = 0; // Number of inhibitor arcs
            bool skipped = false;
        };

        struct Place {
            shared_const_string name;
            const ColorType* type;
            Multiset marking;
            double _x = 0, _y = 0;
            uint32_t inhibitor = 0; // Number of inhibitor arcs
            std::vector<uint32_t> _pre;
            std::vector<uint32_t> _post;
            bool skipped = false;
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

