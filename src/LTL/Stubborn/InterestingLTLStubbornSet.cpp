/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LTL/Stubborn/EvalAndSetVisitor.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "LTL/Stubborn/InterestingLTLStubbornSet.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    bool InterestingLTLStubbornSet::prepare(const PetriEngine::Structures::State *marking) {
        reset();
        _parent = marking;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if (_ordering.empty())
            return false;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return true;
        }
        PQL::EvaluationContext evaluationContext{_parent->marking(), &_net};
        for (auto &q : _queries) {
            EvalAndSetVisitor evalAndSetVisitor{evaluationContext};
            q->visit(evalAndSetVisitor);

            InterestingLTLTransitionVisitor interesting{*this, true};
            q->visit(interesting);
        }
        closure();
        //findKeyTransition();

        ensureRuleV();

        ensureRulesL();

        _nenabled = _ordering.size();


        if (!_has_enabled_stubborn) {
            memset(_stubborn.get(), 1, _net.numberOfTransitions());
        }
        _nenabled = _ordering.size();

        return true;
    }

    uint32_t InterestingLTLStubbornSet::next() {
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

    void InterestingLTLStubbornSet::findKeyTransition() {
        // try to find invisible key transition first
        assert(!_ordering.empty());
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
    void InterestingLTLStubbornSet::ensureRuleV() {
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

    void InterestingLTLStubbornSet::ensureRulesL() {
        static_assert(isRuleVPrime, "Plain rule V does not imply L1");
    }


    void InterestingLTLStubbornSet::reset() {
        StubbornSet::reset();
        _skipped.clear();
        _has_enabled_stubborn = false;
    }

    bool InterestingLTLStubbornSet::generateAll(const LTL::Structures::ProductState *parent) {
        // Ensure rule L2, forcing all visible transitions into the stubborn set when closing cycle.
        for (uint32_t i = 0; i < _net.numberOfTransitions(); ++i) {
            if (_visible[i]) {
                addToStub(i);
            }
        }
        // recompute entire set
        closure();

       /* // re-add previously non-stubborn, enabled transitions to order if they are now stubborn.
        while (!_skipped.empty()) {
            auto tid = _skipped.front();
            if (_stubborn[tid])
                _ordering.push_back(tid);
            _skipped.pop_front();
        }*/
    }

    void InterestingLTLStubbornSet::addToStub(uint32_t t) {
        if (_enabled[t])
            _has_enabled_stubborn = true;
        StubbornSet::addToStub(t);
    }

}