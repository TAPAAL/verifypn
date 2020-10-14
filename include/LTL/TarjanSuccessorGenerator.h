/*
 * File:   TarjanSuccessorGenerator.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#ifndef VERIFYPN_TARJANSUCCESSORGENERATOR_H
#define VERIFYPN_TARJANSUCCESSORGENERATOR_H

#include "LTL/ProductSuccessorGenerator.h"

namespace LTL {
    class TarjanSuccessorGenerator : public ProductSuccessorGenerator {
    public:
        TarjanSuccessorGenerator(const PetriEngine::PetriNet &net, const PetriEngine::PQL::Condition_ptr &cond)
                : ProductSuccessorGenerator(net, cond) {}

        bool next(LTL::Structures::ProductState &write, size_t &tindex);


    };
}

#endif //VERIFYPN_TARJANSUCCESSORGENERATOR_H
