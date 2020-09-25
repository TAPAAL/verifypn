//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_MODELCHECKER_H
#define VERIFYPN_MODELCHECKER_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/ProductSuccessorGenerator.h"

namespace LTL {
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet& net, const PetriEngine::PQL::Condition_ptr);
        virtual bool isSatisfied() = 0;

    protected:
        ProductSuccessorGenerator successorGenerator;
    };
}

#endif //VERIFYPN_MODELCHECKER_H
