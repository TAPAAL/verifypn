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

namespace PetriEngine { namespace PQL {

    /*** Nested Deadlock ***/

    bool hasNestedDeadlock(const Condition_ptr& condition)
    {
        return hasNestedDeadlock(condition.get());
    }

    bool hasNestedDeadlock(const Condition* condition) {
        NestedDeadlockVisitor v;
        Visitor::visit(v, condition);
        return v.getReturnValue();
    }

    void NestedDeadlockVisitor::_accept(const LogicalCondition *condition) {
        _nested_in_logical_condition = true;
        for (auto& c : condition->getOperands())
            Visitor::visit(this, c);
        _nested_in_logical_condition = false;
    }

    void NestedDeadlockVisitor::_accept(const DeadlockCondition *condition) {
        if (_nested_in_logical_condition)
            setConditionFound();
    }

    /*** Is Temporal ***/

    bool isTemporal(const Condition_ptr& condition) {
        return isTemporal(condition.get());
    }

    bool isTemporal(const Condition *condition) {
        IsTemporalVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

    void IsTemporalVisitor::_accept(const SimpleQuantifierCondition *condition) {
        setConditionFound();
    }

    void IsTemporalVisitor::_accept(const UntilCondition *condition) {
        setConditionFound();
    }


    /*** Is Reachability ***/
    bool isReachability(const Condition* condition) {
        IsNotReachabilityVisitor visitor;
        Visitor::visit(visitor, condition);
        return !visitor.getReturnValue();
    }

    bool isReachability(const Condition_ptr& condition) {
        return isReachability(condition.get());
    }


    void IsNotReachabilityVisitor::_accept(const SimpleQuantifierCondition *element) {
        // AG and EF have their own accepts, all other quantifiers are forbidden
        setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const UntilCondition *element) {
        setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const EFCondition *element) {
        if (!_is_nested) {
            _is_nested = true;
            Visitor::visit(this, element->getCond());
            _is_nested = false;
        } else {
            setConditionFound();
        }
    }

    void IsNotReachabilityVisitor ::_accept(const AGCondition *element) {
        if (!_is_nested) {
            _is_nested = true;
            Visitor::visit(this, element->getCond());
            _is_nested = false;
        } else {
            setConditionFound();
        }
    }

    void IsNotReachabilityVisitor::_accept(const ECondition *element) {
        if (!_is_nested) {
            if (auto cond = dynamic_cast<FCondition*>(element->getCond().get())) {
                _is_nested = true;
                Visitor::visit(this, cond->getCond());
                _is_nested = false;
            } else {
                setConditionFound();
            }
        } else {
            setConditionFound();
        }
    }

    void IsNotReachabilityVisitor::_accept(const ACondition *element) {
        if (!_is_nested) {
            if (auto cond = dynamic_cast<GCondition*>(element->getCond().get())) {
                _is_nested = true;
                Visitor::visit(this, cond->getCond());
                _is_nested = false;
            } else {
                setConditionFound();
            }
        } else {
            setConditionFound();
        }
    }

    void IsNotReachabilityVisitor::_accept(const LogicalCondition *element) {
        if (_is_nested) {
            for(auto& c : element->getOperands())
            {
                Visitor::visit(this, c);
                if(_condition_found)
                    break;
            }
        } else {
            setConditionFound();
        }
    }

    void IsNotReachabilityVisitor::_accept(const CompareCondition *element) {
        if (!_is_nested) setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const NotCondition *element) {
        Visitor::visit(this, element->getCond());
    }

    void IsNotReachabilityVisitor::_accept(const BooleanCondition *element) {
        if (!_is_nested) setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const DeadlockCondition *element) {
        if (!_is_nested) setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const UnfoldedUpperBoundsCondition *element) {
        if (!_is_nested) setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const ShallowCondition *element) {
        if (!_is_nested) setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const QuasiLivenessCondition *element) {
        if (element->getCompiled())
            Visitor::visit(this, element->getCompiled());
        else
            setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const LivenessCondition *element) {
        if (element->getCompiled())
            Visitor::visit(this, element->getCompiled());
        else
            setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const StableMarkingCondition *element) {
        if (element->getCompiled())
            Visitor::visit(this, element->getCompiled());
        else
            setConditionFound();
    }

    void IsNotReachabilityVisitor::_accept(const CompareConjunction *element) {
        if (!_is_nested) setConditionFound();
    }


    /*** Is Loop Sensitive ***/
    bool isLoopSensitive(const Condition_ptr& condition) {
        IsLoopSensitiveVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

    void IsLoopSensitiveVisitor::_accept(const ECondition* condition) {
        // Other LTL Loop sensitivity depend on the outermost quantifier being an A,
        // so if it is an E we disable loop sensitive reductions.
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const GCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const FCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const XCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const EXCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const EGCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const AXCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const AFCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const AUCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const DeadlockCondition* condition) {
        setConditionFound();
    }

    void IsLoopSensitiveVisitor::_accept(const ACondition *condition)
    {
        _negated = !_negated;
        AnyVisitor::_accept(condition);
    }

    void IsLoopSensitiveVisitor::_accept(const NotCondition *condition)
    {
        _negated = !_negated;
        AnyVisitor::_accept(condition);
    }

    void IsLoopSensitiveVisitor::_accept(const UntilCondition *condition)
    {
        if(_negated)
            setConditionFound();
        else
            AnyVisitor::_accept(condition);
    }


    /*** Contains Next ***/
    bool containsNext(const Condition_ptr& condition) {
        return containsNext(condition.get());
    }
    bool containsNext(const Condition* condition) {
        ContainsNextVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

    void ContainsNextVisitor::_accept(const XCondition* condition) {
        setConditionFound();
    }

    void ContainsNextVisitor::_accept(const EXCondition* condition) {
        setConditionFound();
    }

    void ContainsNextVisitor::_accept(const AXCondition* condition) {
        setConditionFound();
    }

    bool containsUpperBounds(const Condition_ptr& condition) {
        return containsUpperBounds(condition.get());
    }

    bool containsUpperBounds(const Condition* condition) {
        ContainsUpperBoundsVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

    bool containsDeadlock(const Condition_ptr condition) {
        ContainsDeadlockVisitor visitor;
        Visitor::visit(visitor, condition);
        return visitor.getReturnValue();
    }

} }
