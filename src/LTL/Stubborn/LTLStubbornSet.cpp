/*
 * File:   LTLStubbornSet.cpp.cc
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 08/02/2021
 */

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "LTL/Stubborn/LTLStubbornSet.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    void LTLStubbornSet::prepare(const PetriEngine::Structures::State *marking) {
        _parent = marking;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if (_ordering.size() == 0) return;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return;
        }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
        }
        findKeyTransition();

        ensureRuleV();

        ensureRulesL();

    }

    void LTLStubbornSet::findKeyTransition() {
        // try to find invisible key transition first
        auto tkey = _ordering.front();
        if (_visible[tkey]) {
            for (uint32_t tid = 0; tid < _net.numberOfTransitions(); ++tid) {
                if (_enabled[tid] && !_visible[tid]) {
                    tkey = tid;
                    break;
                }
            }
        }
        addToStub(tkey);

        // include relevant transitions
        auto ptr = transitions()[tkey];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;

        for (; finv < linv; ++finv) {
            auto inv = invariants()[finv];
            // TODO correct?
            presetOf(inv.place, true);
        }
    }

    constexpr bool isRuleVPrime = true;
    void LTLStubbornSet::ensureRuleV() {
        // Rule V: If there is an enabled, visible transition in the stubborn set,
        // all visible transitions must be stubborn.
        // Rule V' (implemented): If there is an enabled, visible transition
        // in the stubborn set, then T_s(s) = T.
        bool visibleStubborn = false;
        for (uint32_t tid = 0; tid < _net.numberOfTransitions(); ++tid) {
            if (_stubborn[tid] && _enabled[tid] && _visible[tid]) {
                visibleStubborn = true; break;
            }
        }
        if (!visibleStubborn) return;
        else {
            for (uint32_t tid=0; tid < _net.numberOfTransitions(); ++tid) {
                _stubborn[tid] = true;
            }
        }
        // following block would implement rule V
        /*for (uint32_t tid = 0; tid < _net.numberOfTransitions(); ++tid) {
            if (_visible[tid]) addToStub(tid);
        }
        closure();*/
    }

    void LTLStubbornSet::ensureRulesL() {
        static_assert(isRuleVPrime, "Plain rule V does not imply L1");
    }
}