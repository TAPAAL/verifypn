//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/LTL_algorithm/ModelChecker.h"

#include <utility>

namespace LTL {
    ModelChecker::ModelChecker(const PetriEngine::PetriNet& net, PetriEngine::PQL::Condition_ptr condition)
        : net(net), formula(condition)
    {

        successorGenerator = std::make_unique<ProductSuccessorGenerator>(net, condition);
        //TODO Create successor generator from net and condition

        //LTL::ProductPrinter::printProduct(*successorGenerator, std::cout, net, condition);
    }
}

