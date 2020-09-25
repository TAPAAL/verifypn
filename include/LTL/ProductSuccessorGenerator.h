//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#ifndef VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
#define VERIFYPN_PRODUCTSUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "LTL/Structures/ProductState.h"
#include "LTL/BuchiSuccessorGenerator.h"
#include "LTL/LTLToBuchi.h"

namespace LTL {
    class ProductSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:
        ProductSuccessorGenerator(const PetriEngine::PetriNet &net,
                                  PetriEngine::PQL::Condition_ptr cond)
                                  : PetriEngine::SuccessorGenerator(net), buchi(makeBuchiAutomaton(cond))
        {}

        void prepare(const LTL::Structures::ProductState *state);

        bool next(LTL::Structures::ProductState &state);

        bool isAccepting(const LTL::Structures::ProductState &state);

    private:
        BuchiSuccessorGenerator buchi;
        bdd cond;
        bool fresh_marking = true;

        bool guard_valid(const PetriEngine::Structures::State &state, bdd &bdd);

        bool next_buchi_succ(LTL::Structures::ProductState &state);
    };

}

#endif //VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
