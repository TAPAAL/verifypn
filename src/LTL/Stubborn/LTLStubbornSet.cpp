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

#ifndef NDEBUG
        std::vector<size_t> stubs;
        for (auto i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_stubborn[i]) {
                stubs.push_back(i);
            }
        }
        if (stubs.empty()) return;
        std::cerr << "Stubborn set is: \n  ";
        for (auto i : stubs) {
            std::cerr << _net.transitionNames()[i] << " ";
        }
        std::cerr << std::endl;
#endif
    }

    uint32_t LTLStubbornSet::next() {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current] && _enabled[_current]) {
                return _current;
            }
            else {
                _skipped.push_back(_current);
            }
        }
        reset();
        return std::numeric_limits<uint32_t>::max();
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
            postsetOf(inv.place, false);
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

    void LTLStubbornSet::reset() {
        StubbornSet::reset();
        _skipped.clear();
    }

    void LTLStubbornSet::generateAll() {
        // Ensure rule L2, forcing all visible transitions into the stubborn set when closing cycle.
        for (uint32_t i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_visible[i]) {
                addToStub(i);
            }
        }
        // recompute entire set
        closure();

        // re-add previously non-stubborn, enabled transitions to order if they are now stubborn.
        while (!_skipped.empty()) {
            auto tid = _skipped.front();
            if (_stubborn[tid])
                _ordering.push_back(tid);
            _skipped.pop_front();
        }
    }


}