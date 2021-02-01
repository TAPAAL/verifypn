/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#ifndef VERIFYPN_QUERYPRINTER_H
#define VERIFYPN_QUERYPRINTER_H

#include "Visitor.h"

#include <iostream>
#include <string>

namespace PetriEngine {
    namespace PQL {
        class QueryPrinter : public Visitor {
        public:
            QueryPrinter(std::ostream &os = std::cout)
                    : os(os) {}

        protected:
            void _accept(const NotCondition *element) override;

            void _accept(const AndCondition *element) override;

            void _accept(const OrCondition *element) override;

            void _accept(const LessThanCondition *element) override;

            void _accept(const LessThanOrEqualCondition *element) override;

            void _accept(const GreaterThanCondition *element) override;

            void _accept(const GreaterThanOrEqualCondition *element) override;

            void _accept(const EqualCondition *element) override;

            void _accept(const NotEqualCondition *element) override;

            void _accept(const DeadlockCondition *element) override;

            void _accept(const CompareConjunction *element) override;

            void _accept(const UnfoldedUpperBoundsCondition *element) override;

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

            void _accept(const UnfoldedFireableCondition *element) override;

            void _accept(const FireableCondition *element) override;

            void _accept(const UpperBoundsCondition *element) override;

            void _accept(const LivenessCondition *element) override;

            void _accept(const KSafeCondition *element) override;

            void _accept(const QuasiLivenessCondition *element) override;

            void _accept(const StableMarkingCondition *element) override;

            void _accept(const BooleanCondition *element) override;

            void _accept(const UnfoldedIdentifierExpr *element) override;

            void _accept(const LiteralExpr *element) override;

            void _accept(const PlusExpr *element) override;

            void _accept(const MultiplyExpr *element) override;

            void _accept(const MinusExpr *element) override;

            void _accept(const SubtractExpr *element) override;

            void _accept(const IdentifierExpr *element) override;


        protected:
            std::ostream &os;

            void _accept(const LogicalCondition *element, const std::string &op);

            void _accept(const CompareCondition *element, const std::string &op);

            void _accept(const NaryExpr *element, const std::string &op);
        };
    }
}

#endif //VERIFYPN_QUERYPRINTER_H
