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

#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Evaluation.h"

namespace PetriEngine {
    bool ReachabilityStubbornSet::prepare(const Structures::State *state) {
        reset();
        _parent = state;
        
        constructEnabled();
        if (_ordering.size() == 0) return false;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return true;
        }
        assert(!_queries.empty());
        for (auto &q : _queries) {
            PetriEngine::PQL::evaluateAndSet(q, PQL::EvaluationContext((*_parent).marking(), &_net));

            assert(_interesting->get_negated() == false);
            q->visit(*_interesting);
        }

        closure();
        return true;
    }
}
