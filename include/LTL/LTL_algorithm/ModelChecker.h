//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_MODELCHECKER_H
#define VERIFYPN_MODELCHECKER_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/ProductSuccessorGenerator.h"
#include "LTL/LTL_algorithm/ProductPrinter.h"

namespace LTL {
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet& net, PetriEngine::PQL::Condition_ptr, const bool shortcircuitweak);
        virtual bool isSatisfied() = 0;
        
        virtual ~ModelChecker() = default;
        bool weakskip = false;
    protected:
        std::unique_ptr<ProductSuccessorGenerator> successorGenerator;
        const PetriEngine::PetriNet &net;
        PetriEngine::PQL::Condition_ptr formula;

        size_t _discovered = 0;
        const bool shortcircuitweak;
        bool is_weak = false;
    };
}

#endif //VERIFYPN_MODELCHECKER_H
