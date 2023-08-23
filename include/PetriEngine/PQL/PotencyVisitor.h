/* Copyright (C) 2023  Malo Dautry
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

#ifndef VERIFYPN_POTENCYVISITOR_H
#define VERIFYPN_POTENCYVISITOR_H

#include "Visitor.h"
#include "PetriEngine/PQL/Simplifier.h"

namespace PetriEngine { namespace PQL {

    /**
     * Copmutes the initial potencies of a query.
     * It computes the lps equations along the AST, and solve the different
     * systems of equations.
     * The result is aggregated in the potencies.
    */
    void initPotencyVisit(std::shared_ptr<Condition> element, SimplificationContext& context, std::vector<uint32_t> &potencies);

    /**
     * Visitor made to compute the initial potencies of a query.
     * It computes the lps equations along the AST, and merge them into one object.
     * The result is stored in the _return_value.lps attribute.
     * It's up to the caller to solve the equations and aggregate the solutions.
    */
    class PotencyVisitor : public Visitor {

    public:
        explicit PotencyVisitor(SimplificationContext& context) :
                _context(context) {}

        RetvalPot get_return_value() { return std::move(_return_value); }

    protected:
        SimplificationContext& _context;
        RetvalPot _return_value;

        RetvalPot simplify_or(const LogicalCondition* element);
        RetvalPot simplify_and(const LogicalCondition *element);

        RetvalPot simplify_AG(RetvalPot &r);
        RetvalPot simplify_AF(RetvalPot &r);
        RetvalPot simplify_AX(RetvalPot &r);

        RetvalPot simplify_EG(RetvalPot &r);
        RetvalPot simplify_EF(RetvalPot &r);
        RetvalPot simplify_EX(RetvalPot &r);

        template <typename Quantifier>
        RetvalPot simplify_simple_quantifier(RetvalPot &r);

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

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const BooleanCondition *element) override;

        void _accept(const PathSelectCondition *element) override;
    };

} }
#endif // VERIFYPN_POTENCYVISITOR_H
