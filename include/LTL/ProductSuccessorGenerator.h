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
                                  const PetriEngine::PQL::Condition_ptr& cond)
                                  : PetriEngine::SuccessorGenerator(net), buchi(makeBuchiAutomaton(cond))
        {}

        [[nodiscard]] size_t initial_buchi_state() const { return buchi.initial_state_number(); };

        void prepare(const LTL::Structures::ProductState *state);

        bool next(LTL::Structures::ProductState &state);

        bool isAccepting(const LTL::Structures::ProductState &state);

        void makeInitialState(std::vector<LTL::Structures::ProductState> &states) {
            auto buf = new PetriEngine::MarkVal[_net.numberOfPlaces() + 1];
            std::copy(_net.initial(), _net.initial() + _net.numberOfPlaces(), buf);
            buf[_net.numberOfPlaces()] = 0;
            LTL::Structures::ProductState state;
            state.setMarking(buf, _net.numberOfPlaces());
            state.setBuchiState(initial_buchi_state());
            buchi.prepare(state.getBuchiState());
            while (next_buchi_succ(state)) {
                states.emplace_back();
                states.back().setMarking(new PetriEngine::MarkVal[_net.numberOfPlaces() + 1], _net.numberOfPlaces());
                std::copy(state.marking(), state.marking() + _net.numberOfPlaces(), states.back().marking());
                states.back().setBuchiState(state.getBuchiState());
            }
        }

    protected:
        bool next(LTL::Structures::ProductState &write, uint32_t &tindex);

    private:
        BuchiSuccessorGenerator buchi;
        bdd cond;
        bool fresh_marking = true;

        bool guard_valid(const PetriEngine::Structures::State &state, bdd bdd);

        bool next_buchi_succ(LTL::Structures::ProductState &state);
    };

}

#endif //VERIFYPN_PRODUCTSUCCESSORGENERATOR_H
