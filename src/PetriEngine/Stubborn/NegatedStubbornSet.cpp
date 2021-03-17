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

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"
#include "PetriEngine/Stubborn/NegatedStubbornSet.h"

namespace PetriEngine {
    void NegatedStubbornSet::prepare(const Structures::State *state, const std::vector<PQL::Condition_ptr> &_queries,
                                     bool do_closure)
    {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        constructEnabled();
        if (_queries.empty())
            return;
        if (_ordering.size() == 0) return;
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return;
        }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
            InterestingTransitionVisitor interesting{*this, do_closure};
            interesting.negate();

            q->visit(interesting);
        }

        if (do_closure)
            closure();
    }

    void NegatedStubbornSet::extend(const PQL::Condition_ptr &query, bool do_closure) {
        query->evalAndSet(PQL::EvaluationContext(_parent->marking(), &_net));
        InterestingTransitionVisitor interesting{*this, do_closure};
        interesting.negate();
        query->visit(interesting);
    }

    void NegatedStubbornSet::addToStub(uint32_t t)
    {
        if (_stubborn[t]) {
            _stubborn[t] = false;
            _unprocessed.push_back(t);
        }
    }

    void NegatedStubbornSet::reset()
    {
        memset(_enabled.get(), false, sizeof(bool) * _net.numberOfTransitions());
        memset(_stubborn.get(), true, sizeof(bool) * _net.numberOfTransitions());
        _ordering.clear();
        _tid = 0;
    }


}