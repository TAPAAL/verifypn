/* Copyright (C) 2011  Rasmus Tollund <rtollu18@student.aau.dk>
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

#ifndef VERIFYPN_FORMULASIZE_H
#define VERIFYPN_FORMULASIZE_H

#include "Visitor.h"

namespace PetriEngine { namespace PQL {
    /** Count size of the entire formula in number of nodes */
    int formulaSize(const Condition_constptr & condition);
    int formulaSize(const Expr_ptr& element);

    class FormulaSizeVisitor : public Visitor {

    public:
        [[nodiscard]] int getReturnValue() const {
            return _return_value;
        }

    private:
        int _return_value = -1;

        int subvisit(const Condition_ptr& condition) {
            Visitor::visit(this, condition);
            return _return_value;
        }
        int subvisit(const Expr_ptr& expr) {
            Visitor::visit(this, expr);
            return _return_value;
        }

        void _accept(const CompareConjunction *condition) override;

        void _accept(const CompareCondition *condition) override;

        void _accept(const BooleanCondition *condition) override;

        void _accept(const DeadlockCondition *condition) override;

        void _accept(const UnfoldedUpperBoundsCondition *condition) override;

        void _accept(const NaryExpr *element) override;

        void _accept(const CommutativeExpr *element) override;

        void _accept(const MinusExpr *element) override;

        void _accept(const LiteralExpr *element) override;

        void _accept(const IdentifierExpr *element) override;

        void _accept(const UnfoldedIdentifierExpr *condition) override;

        void _accept(const ShallowCondition *condition) override;

        void _accept(const NotCondition *condition) override;

        void _accept(const SimpleQuantifierCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const LogicalCondition *condition) override;
    };
} }
#endif //VERIFYPN_FORMULASIZE_H
