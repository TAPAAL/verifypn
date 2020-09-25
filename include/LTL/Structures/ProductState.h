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
    class ProductState : public PetriEngine::Structures::State {
    public:
        //TODO override equality operators to handle both marking and NBA state
        size_t getBuchiState() const {
            return buchi_state;
        }

    private:
        size_t buchi_state;
        friend class LTL::ProductSuccessorGenerator;
    };
}

#endif //VERIFYPN_PRODUCTSTATE_H
