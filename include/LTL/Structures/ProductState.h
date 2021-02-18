/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_PRODUCTSTATE_H
#define VERIFYPN_PRODUCTSTATE_H

#include "PetriEngine/Structures/State.h"

namespace LTL {
    class ProductSuccessorGenerator;
}
namespace LTL::Structures {
    /**
     * State on the form (M, q) for marking M and NBA state q.
     * Represented as array of size nplaces + 1, where the last element is the number
     * of NBA state q, and the first nplaces elements are the actual marking.
     */
    class ProductState : public PetriEngine::Structures::State {
    public:
        ProductState() : PetriEngine::Structures::State() {}

        void setMarking(PetriEngine::MarkVal* marking, size_t nplaces)
        {
            State::setMarking(marking);
            // because zero-indexing
            buchi_state_idx = nplaces;
        }

        //TODO override equality operators to handle both marking and NBA state
        size_t getBuchiState() const {
            return marking()[buchi_state_idx];
        }

        void setBuchiState(size_t state) {
            marking()[buchi_state_idx] = state;
        }

        [[nodiscard]] bool markingEqual(const PetriEngine::MarkVal *rhs) const {
            for (int i = 0; i < buchi_state_idx; ++i) {
                if (marking()[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const ProductState &rhs) const {
            for (int i = 0; i <= buchi_state_idx; ++i) {
                if (marking()[i] != rhs.marking()[i]) {
                    return false;
                }
            }
            return true;
/*            return static_cast<const PetriEngine::Structures::State &>(*this) ==
                   static_cast<const PetriEngine::Structures::State &>(rhs) &&
                   getBuchiState() == rhs.getBuchiState();*/
        }

        size_t size() const { return buchi_state_idx + 1; }

        bool operator!=(const ProductState &rhs) const {
            return !(rhs == *this);
        }
    private:
        friend class LTL::ProductSuccessorGenerator;
        size_t buchi_state_idx;
    };
}

#endif //VERIFYPN_PRODUCTSTATE_H
