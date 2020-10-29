//
// Created by simwir on 09/10/2020.
//

#ifndef VERIFYPN_PRODUCTPRINTER_H
#define VERIFYPN_PRODUCTPRINTER_H

#include "LTL/Structures/ProductStateFactory.h"
#include "LTL/ProductSuccessorGenerator.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/PQL/Contexts.h"

namespace LTL::ProductPrinter {
    void printProduct(ProductSuccessorGenerator &successorGenerator, std::ostream &os, const PetriEngine::PetriNet &net,
                      Condition_ptr formula);
}
#endif //VERIFYPN_PRODUCTPRINTER_H
