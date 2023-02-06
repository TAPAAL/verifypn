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

#ifndef VERIFYPN_FIRECOUNTHEURISTIC_H
#define VERIFYPN_FIRECOUNTHEURISTIC_H

#include "LTL/SuccessorGeneration/Heuristic.h"

namespace LTL {
    class FireCountHeuristic : public Heuristic {
    public:
        explicit FireCountHeuristic(size_t n_trans) : _fireCount(n_trans) {}

        uint32_t eval(const Structures::ProductState &state, uint32_t tid) override {
            assert(tid <= _fireCount.size());
            return _fireCount[tid];
        }

        bool has_heuristic(const Structures::ProductState &state) override {
            return true;
        }

        void push(uint32_t tid) override {
            assert(tid <= _fireCount.size());
            ++_fireCount[tid];
        }

        void pop(uint32_t tid) override {
            assert(tid <= _fireCount.size());
            --_fireCount[tid];
        }

        std::ostream &output(std::ostream &os) {
            return os << "FIRECOUNT_HEUR";
        }


    protected:
        std::vector<uint32_t> _fireCount;
    };
}

#endif //VERIFYPN_FIRECOUNTHEURISTIC_H
