//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
#define VERIFYPN_PRODUCTSUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"

namespace LTL {
class ProductSuccessorGenerator : public PetriEngine::SuccessorGenerator {
public:
    ProductSuccessorGenerator(const PetriEngine::PetriNet& net); // TODO add NBA to constructor parameter
    bool isAccepting(const PetriEngine::Structures::State& state);
};
}

#endif //VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
