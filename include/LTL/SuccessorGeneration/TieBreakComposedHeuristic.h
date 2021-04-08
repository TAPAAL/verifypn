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

#ifndef VERIFYPN_TIEBREAKCOMPOSEDHEURISTIC_H
#define VERIFYPN_TIEBREAKCOMPOSEDHEURISTIC_H

#include "ComposedHeuristic.h"

namespace LTL {

    template<uint8_t S>
    class TieBreakComposedHeuristic: public ComposedHeuristic {
    public:

        TieBreakComposedHeuristic(std::unique_ptr<Heuristic> primary, std::unique_ptr<Heuristic> secondary)
                : ComposedHeuristic(std::move(primary), std::move(secondary)) {}

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override {
            if (_primary_has_heuristic && !_secondary_has_heuristic) {
                return _primary->eval(state, tid);
            } else if (!_primary_has_heuristic && _secondary_has_heuristic) {
                return _secondary->eval(state, tid);
            } else if (_primary_has_heuristic && _secondary_has_heuristic) {
                //std::cerr << "Primary: " << _primary->eval(state, tid) << ", S: " << S << ", Secondary: " << _secondary->eval(state, tid) << ", secondary_mask: " << _secondary_mask << " result: " << ((_primary->eval(state, tid) << S) | (_secondary_mask & _secondary->eval(state, tid))) << std::endl;
                return (_primary->eval(state, tid) << S) | (_secondary_mask & _secondary->eval(state, tid));
            } else {
                return 0;
            }
        }

    private:
        static constexpr uint32_t _secondary_mask = (1 << S) - 1;
    };

}

#endif //VERIFYPN_TIEBREAKCOMPOSEDHEURISTIC_H
