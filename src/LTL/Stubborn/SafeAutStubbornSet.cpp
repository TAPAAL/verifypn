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

#include "LTL/Stubborn/SafeAutStubbornSet.h"

namespace LTL {
    using namespace PetriEngine;

    bool SafeAutStubbornSet::prepare(const LTL::Structures::ProductState *state)
    {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, _net.numberOfPlaces());

        constructEnabled();
        if (_ordering.empty()) {
            return false;
        }
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            return true;
        }

        InterestingTransitionVisitor interesting{*this, false};

        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext((*_parent).marking(), &_net));
            q->visit(interesting);
        }
        assert(!_bad);

        _unsafe.swap(_stubborn);
        //memset(_stubborn.get(), false, sizeof(bool) * _net.numberOfTransitions());
        _unprocessed.clear();
        memset(_places_seen.get(), 0, _net.numberOfPlaces());

        assert(_unprocessed.empty());

        for (auto &q : _queries) {
            q->visit(interesting);
            closure();
            if (_bad) {
                // abort
                set_all_stubborn();
                return true;
            }
        }
        return true;
    }
}