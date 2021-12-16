//
// Created by ragusa on 28/11/2021.
//

#ifndef VERIFYPN_PREDICATECHECKERS_H
#define VERIFYPN_PREDICATECHECKERS_H

#include "Visitor.h"

namespace PetriEngine::PQL {

    bool hasNestedDeadlock(const Condition_ptr& condition);

    class NestedDeadlockVisitor : public AnyVisitor {
    private:
        bool _nested_in_logical_condition = false;

        void _accept(const LogicalCondition* condition) override;

        void _accept(const DeadlockCondition* condition) override;
    };


    bool isTemporal(Condition_ptr condition);
    bool isTemporal(Condition *condition);

    class IsTemporalVisitor : public AnyVisitor {

        void _accept(const SimpleQuantifierCondition *condition) override;

        void _accept(const UntilCondition *condition) override;
    };


    bool isReachability(const Condition_ptr& condition);

    class IsReachabilityVisitor : public AnyVisitor {

    private:
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

    class ContainsNextVisitor : public AnyVisitor {
        void _accept(const XCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;
    };
}

#endif //VERIFYPN_PREDICATECHECKERS_H
