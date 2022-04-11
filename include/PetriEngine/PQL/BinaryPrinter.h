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

#ifndef VERIFYPN_BINARYPRINTER_H
#define VERIFYPN_BINARYPRINTER_H

#include "Visitor.h"

namespace PetriEngine { namespace PQL {
    class BinaryPrinter : public Visitor {
    public:
        explicit BinaryPrinter(std::ostream& os) :
            os(os) {}

    protected:
        std::ostream& os;

        void _accept(const NotCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const BooleanCondition *element) override;

        void _accept(const UnfoldedIdentifierExpr *element) override;

        void _accept(const LiteralExpr *element) override;

        void _accept(const MinusExpr *element) override;

        void _accept(const SubtractExpr *element) override;

        void _accept(const CommutativeExpr *element) override;

        void _accept(const SimpleQuantifierCondition *condition) override;

        void _accept(const LogicalCondition *condition) override;

        void _accept(const CompareCondition *condition) override;

        void _accept(const IdentifierExpr *condition) override;

        void _accept(const ShallowCondition *condition) override;
    };
} }


#endif //VERIFYPN_BINARYPRINTER_H
