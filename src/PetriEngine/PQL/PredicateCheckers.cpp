/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *                     Rasmus Tollund <rtollu18@student.aau.dk>
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

#include "PetriEngine/PQL/PredicateCheckers.h"

namespace PetriEngine::PQL {
    bool hasNestedDeadlock(Condition_ptr condition) {
        NestedDeadlockVisitor v;
        condition->visit(v);
        return v.getReturnValue();
    }

    void NestedDeadlockVisitor::_accept(const LogicalCondition *condition) {
        _nested_in_logical_condition = true;
        for (auto& c : condition->getOperands())
            c->visit(*this);
        _nested_in_logical_condition = false;
    }

    void NestedDeadlockVisitor::_accept(const DeadlockCondition *condition) {
        if (_nested_in_logical_condition)
            setConditionFound();
    }
}
