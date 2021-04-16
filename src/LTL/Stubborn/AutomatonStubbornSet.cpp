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

#include "LTL/Structures/GuardInfo.h"
#include "LTL/Stubborn/AutomatonStubbornSet.h"
#include "LTL/Stubborn/EvalAndSetVisitor.h"
#include "LTL/Stubborn/VisibilityVisitor.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

namespace LTL {
    bool AutomatonStubbornSet::prepare(const LTL::Structures::ProductState *state) {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, sizeof(uint8_t) * _net.numberOfPlaces());
        constructEnabled();
        if (_ordering.empty())
            return false;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return true;
        }

        _nenabled = _ordering.size();

        GuardInfo buchi_state = _state_guards[state->getBuchiState()];

        //Interesting on each progressing formula should give NLG
        PQL::EvaluationContext evaluationContext{_parent->marking(), &_net};
        for (auto &q : buchi_state.progressing) {
            EvalAndSetVisitor evalAndSetVisitor{evaluationContext};
            q.condition->visit(evalAndSetVisitor);

            InterestingLTLTransitionVisitor interesting{*this, false};
            q.condition->visit(interesting);
        }
        //Closure should ensure COM
        closure();

        // Ensure we have a key transition in accepting buchi states.
        if (!_has_enabled_stubborn && buchi_state.is_accepting) {
            for (uint32_t i = 0; i < _net.numberOfTransitions(); ++i) {
                if (!_stubborn[i] && _enabled[i]) {
                    addToStub(i);
                    closure();
                    break;
                }
            }
        }

        // Check condition 3
        if (!_aut.guard_valid(evaluationContext, buchi_state.retarding.decision_diagram)) {
            memset(_stubborn.get(), true,  sizeof(bool) * _net.numberOfTransitions());
            return true;
        }

        auto negated_retarding = std::make_unique<NotCondition>(buchi_state.retarding.condition);
        _retarding_stubborn_set.setQuery(negated_retarding.get());
        _retarding_stubborn_set.prepare(state);


        //Check that S-INV is satisfied
        if (has_shared_mark(_stubborn.get(), _retarding_stubborn_set.stubborn(), _net.numberOfTransitions())){
            memset(_stubborn.get(), true,  sizeof(bool) * _net.numberOfTransitions());
            return true;
        }
        return true;
    }

    uint32_t AutomatonStubbornSet::next() {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if (_stubborn[_current] && _enabled[_current]) {
                return _current;
            }
        }
        reset();
        return std::numeric_limits<uint32_t>::max();
    }

    void AutomatonStubbornSet::reset() {
        StubbornSet::reset();
        _retarding_stubborn_set.reset();
        _has_enabled_stubborn = false;
    }

    void AutomatonStubbornSet::addToStub(uint32_t t) {
        if (_enabled[t])
            _has_enabled_stubborn = true;
        StubbornSet::addToStub(t);
    }
}
