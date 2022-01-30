/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_MUTATINGVISITOR_H
#define VERIFYPN_MUTATINGVISITOR_H

#include "PetriEngine/PQL/Expressions.h"

namespace PetriEngine {
    namespace PQL {
        class MutatingVisitor {
        public:
            MutatingVisitor() {}

            template<typename T>
            void accept(T &&element) {
                _accept(element);
            }

        protected:

            virtual void _accept(NotCondition *element) = 0;

            virtual void _accept(AndCondition *element) = 0;

            virtual void _accept(OrCondition *element) = 0;

            virtual void _accept(LessThanCondition *element) = 0;

            virtual void _accept(LessThanOrEqualCondition *element) = 0;

            virtual void _accept(EqualCondition *element) = 0;

            virtual void _accept(NotEqualCondition *element) = 0;

            virtual void _accept(DeadlockCondition *element) = 0;

            virtual void _accept(CompareConjunction *element) = 0;

            virtual void _accept(UnfoldedUpperBoundsCondition *element) = 0;

            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(ControlCondition *) {
                assert(false);
                throw base_error("No accept for EFCondition");
            };

            virtual void _accept(EFCondition *) {
                assert(false);
                throw base_error("No accept for EFCondition");
            };

            virtual void _accept(EGCondition *) {
                assert(false);
                throw base_error("No accept for EGCondition");
            };

            virtual void _accept(AGCondition *) {
                assert(false);
                throw base_error("No accept for AGCondition");
            };

            virtual void _accept(AFCondition *) {
                assert(false);
                throw base_error("No accept for AFCondition");
            };

            virtual void _accept(EXCondition *) {
                assert(false);
                throw base_error("No accept for EXCondition");
            };

            virtual void _accept(AXCondition *) {
                assert(false);
                throw base_error("No accept for AXCondition");
            };

            virtual void _accept(EUCondition *) {
                assert(false);
                throw base_error("No accept for EUCondition");
            };

            virtual void _accept(AUCondition *) {
                assert(false);
                throw base_error("No accept for AUCondition");
            };

            virtual void _accept(ACondition *) {
                assert(false);
                throw base_error("No accept for ACondition");
            };

            virtual void _accept(ECondition *) {
                assert(false);
                throw base_error("No accept for ECondition");
            };

            virtual void _accept(GCondition *) {
                assert(false);
                throw base_error("No accept for GCondition");
            };

            virtual void _accept(FCondition *) {
                assert(false);
                throw base_error("No accept for FCondition");
            };

            virtual void _accept(XCondition *) {
                assert(false);
                throw base_error("No accept for XCondition");
            };

            virtual void _accept(UntilCondition *) {
                assert(false);
                throw base_error("No accept for UntilCondition");
            };

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(UnfoldedFireableCondition *element) {
                assert(false);
                throw base_error("No accept for UnfoldedFireableCondition");
            };

            virtual void _accept(FireableCondition *element) {
                assert(false);
                throw base_error("No accept for FireableCondition");
            };

            virtual void _accept(UpperBoundsCondition *element) {
                assert(false);
                throw base_error("No accept for UpperBoundsCondition");
            };

            virtual void _accept(LivenessCondition *element) {
                assert(false);
                throw base_error("No accept for LivenessCondition");
            };

            virtual void _accept(KSafeCondition *element) {
                assert(false);
                throw base_error("No accept for KSafeCondition");
            };

            virtual void _accept(QuasiLivenessCondition *element) {
                assert(false);
                throw base_error("No accept for QuasiLivenessCondition");
            };

            virtual void _accept(StableMarkingCondition *element) {
                assert(false);
                throw base_error("No accept for StableMarkingCondition");
            };

            virtual void _accept(BooleanCondition *element) {
                assert(false);
                throw base_error("No accept for BooleanCondition");
            };

            // Expression
            virtual void _accept(UnfoldedIdentifierExpr *element) {
                assert(false);
                throw base_error("No accept for UnfoldedIndentifierExpr");
            };

            virtual void _accept(LiteralExpr *element) {
                assert(false);
                throw base_error("No accept for LiteralExpr");
            };

            virtual void _accept(PlusExpr *element) {
                assert(false);
                throw base_error("No accept for PlusExpr");
            };

            virtual void _accept(MultiplyExpr *element) {
                assert(false);
                throw base_error("No accept for MultiplyExpr");
            };

            virtual void _accept(MinusExpr *element) {
                assert(false);
                throw base_error("No accept for MinusExpr");
            };

            virtual void _accept(SubtractExpr *element) {
                assert(false);
                throw base_error("No accept for SubtractExpr");
            };

            // shallow expression, default to error
            virtual void _accept(IdentifierExpr *element) {
                assert(false);
                throw base_error("No accept for IdentifierExpr");
            };
        };
    }
}
#endif //VERIFYPN_MUTATINGVISITOR_H
