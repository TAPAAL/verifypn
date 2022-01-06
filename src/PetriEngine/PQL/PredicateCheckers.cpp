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

    /*** Nested Deadlock ***/

    bool hasNestedDeadlock(const Condition_ptr& condition) {
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

    /*** Is Temporal ***/

    bool isTemporal(Condition_ptr condition) {
        return isTemporal(condition.get());
    }

    bool isTemporal(Condition *condition) {
        IsTemporalVisitor visitor;
        condition->visit(visitor);
        return visitor.getReturnValue();
    }

    void IsTemporalVisitor::_accept(const SimpleQuantifierCondition *condition) {
        setConditionFound();
    }

    void IsTemporalVisitor::_accept(const UntilCondition *condition) {
        setConditionFound();
    }


    /*** Is Reachability ***/

    bool isReachability(const Condition_ptr& condition) {
        IsReachabilityVisitor visitor;
        condition->visit(visitor);
        return visitor.getReturnValue();
    }

    void IsReachabilityVisitor::_accept(const ControlCondition *element) {
        // Do nothing
    }

    void IsReachabilityVisitor::_accept(const SimpleQuantifierCondition *element) {
        // Do nothing, some simple quantifiers have their own accept
    }

    void IsReachabilityVisitor::_accept(const UntilCondition *element) {
        // Do nothing
    }

    void IsReachabilityVisitor::_accept(const EFCondition *element) {
        if (!_is_nested) {
            _is_nested = true;
            element->getCond()->visit(*this);
        }
    }

    void IsReachabilityVisitor ::_accept(const AGCondition *element) {
        if (!_is_nested) {
            _is_nested = true;
            element->getCond()->visit(*this);
        }
    }

    void IsReachabilityVisitor::_accept(const ECondition *element) {
        if (!_is_nested) {
            if (auto cond = dynamic_cast<FCondition*>(element->getCond().get())) {
                _is_nested = true;
                cond->visit(*this);
            }
        }
    }

    void IsReachabilityVisitor::_accept(const ACondition *element) {
        if (!_is_nested) {
            if (auto cond = dynamic_cast<GCondition*>(element->getCond().get())) {
                _is_nested = true;
                cond->visit(*this);
            }
        }
    }

    void IsReachabilityVisitor::_accept(const LogicalCondition *element) {
        if (_is_nested) {
            for(auto& c : element->getOperands())
            {
                // This breaks the ANY pattern, we need to check that all operands are reachability
                _condition_found = false;
                c->visit(*this);

                if(!_condition_found)
                    break;
            }
        }
    }

    void IsReachabilityVisitor::_accept(const CompareCondition *element) {
        if (_is_nested) setConditionFound();
    }

    void IsReachabilityVisitor::_accept(const NotCondition *element) {
        element->getCond()->visit(*this);
    }

    void IsReachabilityVisitor::_accept(const BooleanCondition *element) {
        if (_is_nested) setConditionFound();
    }

    void IsReachabilityVisitor::_accept(const DeadlockCondition *element) {
        if (_is_nested) setConditionFound();
    }

    void IsReachabilityVisitor::_accept(const UnfoldedUpperBoundsCondition *element) {
        if (_is_nested) setConditionFound();
    }

    void IsReachabilityVisitor::_accept(const ShallowCondition *element) {
        if (_is_nested) setConditionFound();
    }

    void IsReachabilityVisitor::_accept(const QuasiLivenessCondition *element) {
        if (element->getCompiled())
            element->getCompiled()->visit(*this);
    }

    void IsReachabilityVisitor::_accept(const LivenessCondition *element) {
        if (element->getCompiled())
            element->getCompiled()->visit(*this);
    }

    void IsReachabilityVisitor::_accept(const StableMarkingCondition *element) {
        if (element->getCompiled())
            element->getCompiled()->visit(*this);
    }

    void IsReachabilityVisitor::_accept(const CompareConjunction *element) {
        if (_is_nested) setConditionFound();
    }


    /*** Is Loop Sensitive ***/
    bool isLoopSensitive(const Condition_ptr& condition) {
        IsLoopSensitiveVisitor visitor;
        condition->visit(visitor);
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


    /*** Contains Next ***/
    bool containsNext(const Condition_ptr& condition) {
        ContainsNextVisitor visitor;
        condition->visit(visitor);
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

}
