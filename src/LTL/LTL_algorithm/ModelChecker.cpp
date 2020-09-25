//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/ModelChecker.h"

namespace LTL {
    ModelChecker::ModelChecker(const PetriEngine::PetriNet& net, PetriEngine::PQL::Condition_ptr condition)
        : successorGenerator(ProductSuccessorGenerator(net))
    {
        //TODO Create successor generator from net and condition
    }
}

