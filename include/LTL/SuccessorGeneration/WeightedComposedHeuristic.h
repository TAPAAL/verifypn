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

#ifndef VERIFYPN_WEIGHTEDCOMPOSEDHEURISTIC_H
#define VERIFYPN_WEIGHTEDCOMPOSEDHEURISTIC_H

namespace LTL {

    class WeightedComposedHeuristic: public ComposedHeuristic {
    public:

        WeightedComposedHeuristic(std::unique_ptr<Heuristic> primary, std::unique_ptr<Heuristic> secondary, uint32_t primary_weight, uint32_t secondary_weight)
                : ComposedHeuristic(std::move(primary), std::move(secondary)), _primary_weight(primary_weight), _secondary_weight(secondary_weight) {}

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override {
            if (_primary_has_heuristic && !_secondary_has_heuristic) {
                return _primary->eval(state, tid);
            } else if (!_primary_has_heuristic && _secondary_has_heuristic) {
                return _secondary->eval(state, tid);
            } else if (_primary_has_heuristic && _secondary_has_heuristic) {
                return _primary->eval(state, tid) * _primary_weight + _secondary->eval(state, tid) * _secondary_weight;
            } else {
                return 0;
            }
        }


        std::ostream &output(std::ostream &os) {
            os << "WEIGHTED(" << _primary_weight << "*";
            _primary->output(os) << ",";
            os << _secondary_weight << "*";
            _secondary->output(os) << ")";
            return os;
        }
    private:
        uint32_t _primary_weight, _secondary_weight;
    };

}

#endif //VERIFYPN_WEIGHTEDCOMPOSEDHEURISTIC_H
