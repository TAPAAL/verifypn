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

#ifndef VERIFYPN_SUMCOMPOSEDHEURISTIC_H
#define VERIFYPN_SUMCOMPOSEDHEURISTIC_H

#include <ostream>
#include "ComposedHeuristic.h"
namespace LTL {
    class SumComposedHeuristic : public ComposedHeuristic {
    public:
        SumComposedHeuristic(std::unique_ptr<Heuristic> primary, std::unique_ptr<Heuristic> secondary)
                : ComposedHeuristic(std::move(primary), std::move(secondary)) {}

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override {
            uint32_t primary_val = _primary_has_heuristic ? _primary->eval(state, tid) : 0;
            uint32_t secondary_val = _secondary_has_heuristic ? _secondary->eval(state, tid) : 0;
            return primary_val + secondary_val;
        }

        std::ostream &output(std::ostream &os) override {
            os << "SUM(";
            _primary->output(os) << ",";
            _secondary->output(os) << ")";
            return os;
        }
    };
}

#endif //VERIFYPN_SUMCOMPOSEDHEURISTIC_H
