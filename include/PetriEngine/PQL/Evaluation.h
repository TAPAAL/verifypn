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

namespace PetriEngine::PQL {

    int temp_apply(Expr *element, int lhs, int rhs);
    bool temp_apply(Condition *element, int lhs, int rhs);

    class ApplyVisitor : public Visitor {
    public:

        void set_args(int lhs, int rhs) { _lhs = lhs; _rhs = rhs; }

        [[nodiscard]] int get_return_value() const { return _return_value; }

    private:
        int _lhs;
        int _rhs;
        int _return_value;

        void _accept(const PlusExpr *element) override;

        void _accept(const MultiplyExpr *element) override;

        void _accept(const SubtractExpr *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const LessThanCondition *element) override;

        void _accept(const LessThanOrEqualCondition *element) override;
    };

    int evaluate(Expr *element, const EvaluationContext& context);
    Condition::Result evaluate(Condition *element, const EvaluationContext& context);

    class EvaluationVisitor : public MutatingVisitor {
    public:
        EvaluationVisitor(const EvaluationContext context) : _context(context) {}

        union EvaluationReturnType {
            int _value;
            Condition::Result _result;
        };

        EvaluationReturnType get_return_value();

    private:

        const EvaluationContext _context;
        EvaluationReturnType _return_value;
        ApplyVisitor _apply_visitor;

        bool apply(const Condition* element, int lhs, int rhs);
        int apply(const Expr* element, int lhs, int rhs);

        int32_t pre_op(const NaryExpr *element);

        int32_t pre_op(const CommutativeExpr *element);

        void _accept(UnfoldedIdentifierExpr *element) override;

        void _accept(LiteralExpr *element) override;

        void _accept(MinusExpr *element) override;

        void _accept(CommutativeExpr *element) override;

        void _accept(NaryExpr *element) override;

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

        void _accept(CompareCondition *element) override;

        void _accept(NotCondition *element) override;

        void _accept(BooleanCondition *element) override;

        void _accept(DeadlockCondition *element) override;

        void _accept(UnfoldedUpperBoundsCondition *element) override;

        void _accept(IdentifierExpr *element) override;

        void _accept(ShallowCondition *element) override;
    };


}

#endif //VERIFYPN_EVALUATION_H
