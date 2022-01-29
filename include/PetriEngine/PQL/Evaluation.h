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

    class ApplyVisitor : public Visitor {
    public:

        void set_args(int lhs, int rhs) { _lhs = lhs; _rhs = rhs; }

        [[nodiscard]] int get_return_value() const { return _return_value; }

    private:
        int _lhs;
        int _rhs;
        int _return_value;

        void _accept(const PlusExpr *element);

        void _accept(const MultiplyExpr *element);

        void _accept(const SubtractExpr *element);

        void _accept(const EqualCondition *element);

        void _accept(const NotEqualCondition *element);

        void _accept(const LessThanCondition *element);

        void _accept(const LessThanOrEqualCondition *element);
    };

    class EvaluationVisitor : public MutatingVisitor {
    public:
        EvaluationVisitor(EvaluationContext context) : _context(context) {}

        union EvaluationReturnType {
            int _value;
            Condition::Result _result;
        };

        EvaluationReturnType get_return_value();

    private:

        EvaluationContext _context;
        EvaluationReturnType _return_value;
        ApplyVisitor _apply_visitor;

        bool apply(const Condition* element, int lhs, int rhs);
        int apply(const Expr* element, int lhs, int rhs);

        int32_t pre_op(const NaryExpr *element);

        int32_t pre_op(const CommutativeExpr *element);

        void _accept(UnfoldedIdentifierExpr *element);

        void _accept(LiteralExpr *element);

        void _accept(MinusExpr *element);

        void _accept(CommutativeExpr *element);

        void _accept(NaryExpr *element);

        void _accept(SimpleQuantifierCondition *element);

        void _accept(EGCondition *element);

        void _accept(AGCondition *element);

        void _accept(ControlCondition *element);

        void _accept(EFCondition *element);

        void _accept(AFCondition *element);

        void _accept(ACondition *element);

        void _accept(ECondition *element);

        void _accept(FCondition *element);

        void _accept(GCondition *element);

        void _accept(UntilCondition *element);

        void _accept(AndCondition *element);

        void _accept(OrCondition *element);

        void _accept(CompareConjunction *element);

        void _accept(CompareCondition *element);

        void _accept(NotCondition *element);

        void _accept(BooleanCondition *element);

        void _accept(DeadlockCondition *element);

        size_t unfolded_upper_bounds_value(UnfoldedUpperBoundsCondition *element, const MarkVal *marking);

        void _accept(UnfoldedUpperBoundsCondition *element);
    };


}

#endif //VERIFYPN_EVALUATION_H
