/* Copyright (C) 2011  Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>
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
    int64_t formulaSize(const Condition_constptr & condition);
    int64_t formulaSize(const Expr_ptr& element);

    class FormulaSizeVisitor : public Visitor {

    public:
        [[nodiscard]] int64_t size() const {
            return _return_size;
        }

        [[nodiscard]] int64_t depth() const {
            return _return_depth;
        }

    private:
        int64_t _return_size = -1;
        int64_t _return_depth = -1;

        std::pair<int64_t,int64_t> subvisit(const Condition_ptr& condition) {
            Visitor::visit(this, condition);
            return std::make_pair(_return_size, _return_depth);
        }
        std::pair<int64_t,int64_t> subvisit(const Expr_ptr& expr) {
            Visitor::visit(this, expr);
            return std::make_pair(_return_size, _return_depth);
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

        void _accept(const PathSelectCondition*) override;

        void _accept(const PathQuant*) override;

        void _accept(const PathSelectExpr*) override;
    };
} }
#endif //VERIFYPN_FORMULASIZE_H
