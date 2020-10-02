//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

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

        bool operator!=(const ProductState &rhs) const {
            return !(rhs == *this);
        }

    private:
        size_t buchi_state_idx;
    };
}

#endif //VERIFYPN_PRODUCTSTATE_H
