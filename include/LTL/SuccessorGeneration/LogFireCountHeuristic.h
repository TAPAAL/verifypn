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

#ifndef VERIFYPN_LOGFIRECOUNTHEURISTIC_H
#define VERIFYPN_LOGFIRECOUNTHEURISTIC_H

#include "Heuristic.h"
#include "FireCountHeuristic.h"

namespace LTL {
    constexpr int _approx_log(uint32_t i)
    {
        // log2 of an integer is the highest set bit
        // loop is not optimal but portable and likely reasonably performant for small-ish ints
        // (which most of the time is the expected input)
        int r = 0;
        while (i >>= 1) {
            ++r;
        }
        return r;
    }

    /**
     * Piece-wise zero/logarithmic fire count heuristic, switching to log at threshold defined by parameter
     * (default 200). Intent is to avoid firing the same transition excessively in models with massive token counts.
     */
    class LogFireCountHeuristic : public FireCountHeuristic {
    public:
        explicit LogFireCountHeuristic(size_t n_trans, uint32_t threshold = 200)
                : FireCountHeuristic(n_trans), _threshold(threshold) {}

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override
        {
            if (_fireCount[tid] < _threshold) return 0;
            return _approx_log(_fireCount[tid] - _threshold);
        }

        std::ostream &output(std::ostream &os) {
            return os << "LOGFIRECOUNT_HEUR(" << _threshold << ")";
        }

    private:
        uint32_t _threshold;

        // compile-time sanity checks, feel free to remove if problematic
        static_assert(_approx_log(1) == 0);
        static_assert(_approx_log(2) == 1);
        static_assert(_approx_log(3) == 1);
        static_assert(_approx_log(9) == 3);
        static_assert(_approx_log(15) == 3);
        static_assert(_approx_log(16) == 4);
    };
}

#endif //VERIFYPN_LOGFIRECOUNTHEURISTIC_H
