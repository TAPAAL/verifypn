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

#ifndef VERIFYPN_EVALUATION_H
#define VERIFYPN_EVALUATION_H

#include "MutatingVisitor.h"
#include "Visitor.h"
#include "Contexts.h"

namespace PetriEngine { namespace PQL {

    class BaseEvaluationVisitor : public MutatingVisitor {
    public:
        union ReturnType {
            int64_t _value;
            Condition::Result _result;
        };
        const ReturnType& get_return_value();

    protected:
        virtual void _accept(PlusExpr *element) override final;

        virtual void _accept(MultiplyExpr *element) override final;

        virtual void _accept(SubtractExpr *element) override final;

        virtual void _accept(MinusExpr *element) override final;

        virtual void _accept(UnfoldedIdentifierExpr *element) override final;

        virtual void _accept(IdentifierExpr *element) override final;

        virtual void _accept(LiteralExpr *element) override final;

        explicit BaseEvaluationVisitor(const EvaluationContext& context) : _context(context) {}

    protected:

        const EvaluationContext& _context;
        ReturnType _return_value{};
    };

    int64_t evaluate(Expr *element, const EvaluationContext& context);
    Condition::Result evaluate(Condition *element, const EvaluationContext& context);


    class EvaluateVisitor : public BaseEvaluationVisitor {
    public:
        explicit EvaluateVisitor(const EvaluationContext& context) : BaseEvaluationVisitor(context) {}

    private:
        void _accept(SimpleQuantifierCondition *element) override;

        void _accept(EGCondition *element) override;

        void _accept(AGCondition *element) override;

        void _accept(ControlCondition *element) override;

        void _accept(EFCondition *element) override;

        void _accept(AFCondition *element) override;

        void _accept(ACondition *element) override;

        void _accept(ECondition *element) override;

        void _accept(FCondition *element) override;

        void _accept(GCondition *element) override;

        void _accept(UntilCondition *element) override;

        void _accept(AndCondition *element) override;

        void _accept(OrCondition *element) override;

        void _accept(CompareConjunction *element) override;

        void _accept(NotEqualCondition *element) override;

        void _accept(EqualCondition *element) override;

        void _accept(LessThanCondition *element) override;

        void _accept(LessThanOrEqualCondition *element) override;

        void _accept(NotCondition *element) override;

        void _accept(BooleanCondition *element) override;

        void _accept(DeadlockCondition *element) override;

        void _accept(UnfoldedUpperBoundsCondition *element) override;

        void _accept(ShallowCondition *element) override;
    };

    int64_t evaluateAndSet(Expr *element, const EvaluationContext &context);
    Condition::Result evaluateAndSet(Condition *element, const EvaluationContext &context);

    class EvaluateAndSetVisitor : public BaseEvaluationVisitor {
    public:
        explicit EvaluateAndSetVisitor(const EvaluationContext& context) : BaseEvaluationVisitor(context) {}

    private:
        void _accept(SimpleQuantifierCondition *element) override;

        void _accept(GCondition *element) override;

        void _accept(FCondition *element) override;

        void _accept(EGCondition *element) override;

        void _accept(AGCondition *element) override;

        void _accept(EFCondition *element) override;

        void _accept(AFCondition *element) override;

        void _accept(UntilCondition *element) override;

        void _accept(AndCondition *element) override;

        void _accept(OrCondition *element) override;

        void _accept(CompareConjunction *element) override;

        void _accept(NotEqualCondition *element) override;

        void _accept(EqualCondition *element) override;

        void _accept(LessThanCondition *element) override;

        void _accept(LessThanOrEqualCondition *element) override;

        void _accept(NotCondition *element) override;

        void _accept(BooleanCondition *element) override;

        void _accept(DeadlockCondition *element) override;

        void _accept(UnfoldedUpperBoundsCondition *element) override;

        void _accept(ShallowCondition *element) override;

        void _accept(Expr *element) override;
    };
} }

#endif //VERIFYPN_EVALUATION_H
