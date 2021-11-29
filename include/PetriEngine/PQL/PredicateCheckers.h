//
// Created by ragusa on 28/11/2021.
//

#ifndef VERIFYPN_PREDICATECHECKERS_H
#define VERIFYPN_PREDICATECHECKERS_H

#include "Visitor.h"

namespace PetriEngine::PQL {

    bool hasNestedDeadlock(Condition_ptr condition);

    class NestedDeadlockVisitor : public AnyVisitor {

    private:
        bool _nested_in_logical_condition = false;

        void _accept(const LogicalCondition* condition) override;

        void _accept(const DeadlockCondition* condition) override;
    };
}

#endif //VERIFYPN_PREDICATECHECKERS_H
