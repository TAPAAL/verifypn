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

#ifndef VERIFYPN_COMPOSEDHEURISTIC_H
#define VERIFYPN_COMPOSEDHEURISTIC_H

#include "Heuristic.h"

#include <utility>

namespace LTL {

    class ComposedHeuristic: public Heuristic {
    public:

        ComposedHeuristic(std::unique_ptr<Heuristic> primary, std::unique_ptr<Heuristic> secondary)
                : _primary(std::move(primary)), _secondary(std::move(secondary)) {}

        void prepare(const Structures::ProductState &state) override {
            _primary->prepare(state);
            _secondary->prepare(state);
        }

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override = 0;

        bool has_heuristic(const Structures::ProductState &state) override {
            _primary_has_heuristic = _primary->has_heuristic(state);
            _secondary_has_heuristic = _secondary->has_heuristic(state);
            return _primary_has_heuristic || _secondary_has_heuristic;
        }

    protected:
        std::unique_ptr<Heuristic> _primary, _secondary;
        bool _primary_has_heuristic = false, _secondary_has_heuristic = false;
    };

}

#endif //VERIFYPN_COMPOSEDHEURISTIC_H
