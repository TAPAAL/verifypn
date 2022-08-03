/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_CLONINGVISITOR_H
#define VERIFYPN_CLONINGVISITOR_H

#include "ColorExpressionVisitor.h"
#include "Expressions.h"

namespace PetriEngine {
    namespace Colored {
        class CloningVisitor : public ColorExpressionVisitor {
        protected:
            AllExpression_ptr _all_res;
            ColorExpression_ptr _col_res;
            ArcExpression_ptr _arc_res;
            GuardExpression_ptr _guard_res;
        public:
            virtual void accept(const DotConstantExpression *expression) override;

            virtual void accept(const VariableExpression *expression) override;

            virtual void accept(const UserOperatorExpression *expression) override;

            virtual void accept(const SuccessorExpression *expression) override;

            virtual void accept(const PredecessorExpression *expression) override;

            virtual void accept(const TupleExpression *expression) override;

            virtual void accept(const LessThanExpression *expression) override;

            virtual void accept(const LessThanEqExpression *expression) override;

            virtual void accept(const EqualityExpression *expression) override;

            virtual void accept(const InequalityExpression *expression) override;

            virtual void accept(const AndExpression *expression) override;

            virtual void accept(const OrExpression *expression) override;

            virtual void accept(const AllExpression *expression) override;

            virtual void accept(const NumberOfExpression *expression) override;

            virtual void accept(const AddExpression *expression) override;

            virtual void accept(const SubtractExpression *expression) override;

            virtual void accept(const ScalarProductExpression *expression) override;
        };
    }
}

#endif //VERIFYPN_CLONINGVISITOR_H
