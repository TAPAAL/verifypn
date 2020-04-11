/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeContext.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 5:01 PM
 */

#ifndef RANGECONTEXT_H
#define RANGECONTEXT_H

#include "PetriEngine/TAR/range.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"

#include <type_traits>

namespace PetriEngine
{
    using namespace Reachability;

    using namespace PQL;
    class RangeContext : public Visitor {
    public:
        RangeContext(prvector_t& vector, MarkVal* base, const PetriNet& net, const uint64_t* uses, MarkVal* marking);
                
    private:
        
        void handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict);
                
        prvector_t& _ranges;
        MarkVal* _base;
        const PetriNet& _net;
        const uint64_t* _uses;
        int64_t _limit = 0;
        bool _lt = false;
        MarkVal* _marking;
    protected:

        virtual void _accept(const NotCondition* element) override;
        virtual void _accept(const PetriEngine::PQL::AndCondition* element) override;
        virtual void _accept(const OrCondition* element) override;
        virtual void _accept(const LessThanCondition* element) override;
        virtual void _accept(const LessThanOrEqualCondition* element) override;
        virtual void _accept(const GreaterThanCondition* element) override;
        virtual void _accept(const GreaterThanOrEqualCondition* element) override;
        virtual void _accept(const IdentifierExpr* element) override;
        virtual void _accept(const LiteralExpr* element) override;
        virtual void _accept(const UnfoldedIdentifierExpr* element) override;
        virtual void _accept(const PlusExpr* element) override;
        virtual void _accept(const DeadlockCondition* element) override;
        virtual void _accept(const CompareConjunction* element) override;
    };
}

#endif /* RANGECONTEXT_H */

