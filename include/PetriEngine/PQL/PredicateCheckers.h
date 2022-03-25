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

#ifndef VERIFYPN_PREDICATECHECKERS_H
#define VERIFYPN_PREDICATECHECKERS_H

#include "Visitor.h"

namespace PetriEngine { namespace PQL {

    bool hasNestedDeadlock(const Condition* condition);
    bool hasNestedDeadlock(const Condition_ptr& condition);


    class NestedDeadlockVisitor : public AnyVisitor {
    private:
        bool _nested_in_logical_condition = false;

        void _accept(const LogicalCondition* condition) override;

        void _accept(const DeadlockCondition* condition) override;
    };


    bool isTemporal(const Condition_ptr& condition);
    bool isTemporal(const Condition *condition);

    class IsTemporalVisitor : public AnyVisitor {

        void _accept(const SimpleQuantifierCondition *condition) override;

        void _accept(const UntilCondition *condition) override;
    };


    bool isReachability(const Condition* condition);
    bool isReachability(const Condition_ptr& condition);

    // Check if it is NOT reachability
    class IsNotReachabilityVisitor : public AnyVisitor {

    private:
        // If it is currently nested inside an EF or AG quantifier
        bool _is_nested = false;

        void _accept(const EFCondition *element) override;

        void _accept(const SimpleQuantifierCondition *element) override;

        void _accept(const AGCondition *element) override;

        void _accept(const ECondition *element) override;

        void _accept(const ACondition *element) override;

        void _accept(const LogicalCondition *element) override;

        void _accept(const NotCondition *element) override;

        void _accept(const CompareCondition *element) override;

        void _accept(const BooleanCondition *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const QuasiLivenessCondition *element) override;

        void _accept(const LivenessCondition *element) override;

        void _accept(const StableMarkingCondition *element) override;

        void _accept(const ShallowCondition *element) override;

        void _accept(const UntilCondition *element) override;

        void _accept(const CompareConjunction *element) override;
    };


    bool isLoopSensitive(const Condition_ptr& condition);

    class IsLoopSensitiveVisitor : public AnyVisitor {

        void _accept(const ECondition *condition) override;

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const AUCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const DeadlockCondition *condition) override;
    };


    bool containsNext(const Condition_ptr& condition);
    bool containsNext(const Condition* condition);

    class ContainsNextVisitor : public AnyVisitor {
        void _accept(const XCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;
    };

    class ContainsFireabilityVisitor : public AnyVisitor {
        void _accept(const FireableCondition* c) override
        {
            setConditionFound();
        }

        void _accept(const UnfoldedFireableCondition* c) override
        {
            setConditionFound();
        }
    };

    class ContainsUpperBoundsVisitor : public AnyVisitor {
        void _accept(const UpperBoundsCondition* c) override
        {
            setConditionFound();
        }

        void _accept(const UnfoldedUpperBoundsCondition* c) override
        {
            setConditionFound();
        }
    };

    bool containsUpperBounds(const Condition* condition);
    bool containsUpperBounds(const Condition_ptr& condition);
} }

#endif //VERIFYPN_PREDICATECHECKERS_H
