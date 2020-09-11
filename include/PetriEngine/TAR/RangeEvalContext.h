/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   RangeEvalContext.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 11, 2020, 1:36 PM
 */

#ifndef RANGEEVALCONTEXT_H
#define RANGEEVALCONTEXT_H
#include "PetriEngine/TAR/range.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"

#include <type_traits>

namespace PetriEngine {
    using namespace Reachability;

    using namespace PQL;

    class RangeEvalContext : public Visitor {
    public:
        RangeEvalContext(const prvector_t& vector, const PetriNet& net, const uint64_t* use_count);
        bool satisfied() const { return _bool_result; }
        const prvector_t& constraint() const { return _sufficient; }
    private:

        void handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict);
        const prvector_t& _ranges;
        const PetriNet& _net;
        const uint64_t* _use_count;
        int64_t _literal;
        bool _bool_result;
        prvector_t _sufficient;
        std::vector<uint32_t> _places;
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
        virtual void _accept(const MultiplyExpr* element) override;
        virtual void _accept(const MinusExpr* element) override;
        virtual void _accept(const SubtractExpr* element) override;        

    };
}

#endif /* RANGEEVALCONTEXT_H */

