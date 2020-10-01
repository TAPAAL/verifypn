/*
 * File:   ProductStateFactory.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 30/09/2020
 */

#ifndef VERIFYPN_PRODUCTSTATEFACTORY_H
#define VERIFYPN_PRODUCTSTATEFACTORY_H

#include "LTL/Structures/ProductState.h"

#include <memory>

namespace LTL::Structures{
    class ProductStateFactory {
    public:
        ProductStateFactory(const PetriEngine::PetriNet &net, size_t initial_buchi_state)
            : net(net), buchi_init(initial_buchi_state) {}

        ProductState makeInitialState() {
            auto buf = new PetriEngine::MarkVal[net.numberOfPlaces()+1];
            std::copy(net.initial(), net.initial() + net.numberOfPlaces(), buf);
            buf[net.numberOfPlaces()] = 0;
            ProductState state;
            state.setMarking(buf, net.numberOfPlaces());
            state.setBuchiState(buchi_init);
            return state;
        }

        std::shared_ptr<LTL::Structures::ProductState>
        copy_state(const LTL::Structures::ProductState &state) {
            auto newstate = std::make_shared<LTL::Structures::ProductState>(state);
            auto marking = new PetriEngine::MarkVal[net.numberOfPlaces() + 1];
            auto old = state.marking();
            // +1 to also set buchi state
            std::copy(old, old + net.numberOfPlaces() + 1, marking);
            newstate->setMarking(marking, net.numberOfPlaces());
            return newstate;
        }
    private:
        const PetriEngine::PetriNet &net;
        const size_t buchi_init;
    };
}
#endif //VERIFYPN_PRODUCTSTATEFACTORY_H
