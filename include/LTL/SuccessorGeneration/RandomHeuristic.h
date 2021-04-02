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

#ifndef VERIFYPN_RANDOMHEURISTIC_H
#define VERIFYPN_RANDOMHEURISTIC_H

#include "LTL/SuccessorGeneration/Heuristic.h"

#include <random>

namespace LTL {
    class RandomHeuristic : public Heuristic {
    public:
        explicit RandomHeuristic(uint32_t seed = 0) : g(seed == 0 ? rd() : seed) {}

        uint32_t eval(const LTL::Structures::ProductState &, uint32_t) override {
            return g();
        }


    private:
        std::random_device rd;
        std::mt19937 g;
    };
}

#endif //VERIFYPN_RANDOMHEURISTIC_H
