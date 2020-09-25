//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_PRODUCTSTATE_H
#define VERIFYPN_PRODUCTSTATE_H

#include "PetriEngine/Structures/State.h"

namespace LTL {
    class ProductState : PetriEngine::Structures::State {
        //TODO override equality operators to handle both marking and NBA state
    };
}

#endif //VERIFYPN_PRODUCTSTATE_H
