/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *                     Rasmus Grønkjær Tollund <rasmusgtollund@gmail.com>
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
#ifndef VERIFYPN_SIMPLIFIER_H
#define VERIFYPN_SIMPLIFIER_H

#include "Visitor.h"

namespace PetriEngine::PQL {

    Retval simplify(std::shared_ptr<Condition> element, SimplificationContext& context);

    class Simplifier : public Visitor {

    public:
        explicit Simplifier(SimplificationContext& context) :
                _context(context) {}

        Retval get_return_value() { return std::move(_return_value); }

    protected:
        SimplificationContext& _context;
        Retval _return_value;

        Retval simplify_or(const LogicalCondition* element);
        Retval simplify_and(const LogicalCondition *element);

        Retval simplify_AG(Retval &r);
        Retval simplify_AF(Retval &r);
        Retval simplify_AX(Retval &r);

        Retval simplify_EG(Retval &r);
        Retval simplify_EF(Retval &r);
        Retval simplify_EX(Retval &r);

        template <typename Quantifier>
        Retval simplify_simple_quantifier(Retval &r);

        void _accept(const NotCondition *element) override;

        void _accept(const AndCondition *element) override;

        void _accept(const OrCondition *element) override;

        void _accept(const LessThanCondition *element) override;

        void _accept(const LessThanOrEqualCondition *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const ControlCondition *condition) override;

        void _accept(const EFCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const AGCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const EUCondition *condition) override;

        void _accept(const AUCondition *condition) override;

        void _accept(const ACondition *condition) override;

        void _accept(const ECondition *condition) override;

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const BooleanCondition *element) override;
    };

    Member constraint(const Expr *element, const SimplificationContext &context);

    class ConstraintVisitor : public ExpressionVisitor {
    public:
        explicit ConstraintVisitor(const SimplificationContext &context) : _context(context) {}

        Member get_return_value() { return _return_value; }

    private:
        const SimplificationContext& _context;
        Member _return_value;

        void _accept(const LiteralExpr *element) override;

        void _accept(const UnfoldedIdentifierExpr *element) override;

        void _accept(const PlusExpr *element) override;

        void _accept(const SubtractExpr *element) override;

        void _accept(const MultiplyExpr *element) override;

        void _accept(const MinusExpr *element) override;

        void _commutative_cons(const CommutativeExpr *element, int _constant, const std::function<void(Member &, Member)> &op);

        void _accept(const IdentifierExpr *element) override;
    };
}
#endif //VERIFYPN_SIMPLIFIER_H
