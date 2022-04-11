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
#include "LTL/Structures/BuchiAutomaton.h"

namespace LTL {
    template <class SuccessorGen>
    class ProductSuccessorGenerator;
}
namespace LTL { namespace Structures {
    /**
     * State on the form (M, q) for marking M and NBA state q.
     * Represented as array of size nplaces + 1, where the last element is the number
     * of NBA state q, and the first nplaces elements are the actual marking.
     */
    class ProductState : public PetriEngine::Structures::State {
    public:
        explicit ProductState(const BuchiAutomaton* aut = nullptr) : PetriEngine::Structures::State(), _aut(aut) {}

        void setMarking(PetriEngine::MarkVal* marking, size_t nplaces)
        {
            State::setMarking(marking);
            // because zero-indexing
            _buchi_state_idx = nplaces;
        }

        uint32_t get_buchi_state() const {
            return marking()[_buchi_state_idx];
        }

        void set_buchi_state(uint32_t state) {
            marking()[_buchi_state_idx] = state;
        }

        [[nodiscard]] bool markingEqual(const PetriEngine::MarkVal *rhs) const {
            for (size_t i = 0; i < _buchi_state_idx; ++i) {
                if (marking()[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const ProductState &rhs) const {
            for (size_t i = 0; i <= _buchi_state_idx; ++i) {
                if (marking()[i] != rhs.marking()[i]) {
                    return false;
                }
            }
            return true;
        }


        size_t size() const { return _buchi_state_idx + 1; }


        bool operator!=(const ProductState &rhs) const {
            return !(rhs == *this);
        }

        [[nodiscard]] bool is_accepting() const {
            assert(_aut);
            return _aut->buchi().state_is_accepting(get_buchi_state());
        }

    private:
        template <typename T>
        friend class LTL::ProductSuccessorGenerator;
        size_t _buchi_state_idx;
        const LTL::Structures::BuchiAutomaton* _aut = nullptr;
    };
} }

#endif //VERIFYPN_PRODUCTSTATE_H
