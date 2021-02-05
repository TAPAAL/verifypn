/*
 * File:   ReachabilityStubbornSet.cpp.cc
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 03/02/2021
 */

#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    void ReachabilityStubbornSet::prepare(const Structures::State *state) {
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if (_ordering.size() == 0) return;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return;
        }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
            InterestingTransitionVisitor interesting{*this};

            q->visit(interesting);
        }

        closure();
    }
}
