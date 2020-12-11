//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/Algorithm/ModelChecker.h"

#include <utility>

namespace LTL {
    ModelChecker::ModelChecker(const PetriEngine::PetriNet& net, PetriEngine::PQL::Condition_ptr condition, const bool shortcircuitweak)
        : net(net), formula(condition), shortcircuitweak(shortcircuitweak)
    {

        successorGenerator = std::make_unique<ProductSuccessorGenerator>(net, condition);
        //TODO Create successor generator from net and condition

        //LTL::ProductPrinter::printProduct(*successorGenerator, std::cout, net, condition);
    }
}

