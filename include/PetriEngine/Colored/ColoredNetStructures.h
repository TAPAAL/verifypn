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

#include <vector>
#include <set>
#include <assert.h>
#include "Colors.h"
#include "Expressions.h"
#include "Multiset.h"

namespace PetriEngine {
    namespace Colored {
        
        struct Arc {
            uint32_t place;
            uint32_t transition;
            ArcExpression_ptr expr;
            bool input;
        };
        
        struct Transition {
            std::string name;
            GuardExpression_ptr guard;
            std::vector<Arc> input_arcs;
            std::vector<Arc> output_arcs;
            std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>> variableMaps;
            bool considered;
        };
        
        struct Place {
            std::string name;
            ColorType* type;
            Multiset marking;
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

