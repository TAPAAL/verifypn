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
        RangeEvalContext(const prvector_t& vector, const PetriNet& net);
        bool satisfied() const { return _bool_result; }
        const std::vector<bool>& used_places() const { return _in_use; }
    private:

        void handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict);
        const prvector_t& _ranges;
        const PetriNet& _net;
        int64_t _upper_result;
        int64_t _lower_result;
        bool _bool_result;
        std::vector<bool> _in_use;
    protected:

        virtual void _accept(const NotCondition* element) override;
        virtual void _accept(const AndCondition* element) override;
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

#endif /* RANGEEVALCONTEXT_H */

