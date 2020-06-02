/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   PlaceUseVisitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 23, 2020, 8:44 PM
 */

#ifndef PLACEUSEVISITOR_H
#define PLACEUSEVISITOR_H

#include "PetriEngine/PQL/Visitor.h"

#include <vector>

namespace PetriEngine {
namespace PQL {
    class PlaceUseVisitor : public Visitor {
    private:
        std::vector<bool> _in_use;
    public:

        PlaceUseVisitor(size_t places);

        const std::vector<bool> in_use() const {
            return _in_use;
        }
    protected:
        virtual void _accept(const NotCondition* element) override;
        virtual void _accept(const AndCondition* element) override;
        virtual void _accept(const OrCondition* element) override;
        virtual void _accept(const LessThanCondition* element) override;
        virtual void _accept(const LessThanOrEqualCondition* element) override;
        virtual void _accept(const GreaterThanCondition* element) override;
        virtual void _accept(const GreaterThanOrEqualCondition* element) override;
        virtual void _accept(const EqualCondition* element) override;
        virtual void _accept(const NotEqualCondition* element) override;
        virtual void _accept(const LiteralExpr* element) override;
        virtual void _accept(const UnfoldedIdentifierExpr* element) override;
        virtual void _accept(const PlusExpr* element) override;
        virtual void _accept(const DeadlockCondition* element) override;
        virtual void _accept(const CompareConjunction* element) override;
        virtual void _accept(const UnfoldedUpperBoundsCondition* element) override;
    };
}
}

#endif /* PLACEUSEVISITOR_H */

