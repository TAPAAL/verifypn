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
            virtual void _accept(EFCondition *) {
                assert(false);
                std::cerr << "No accept for EFCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(EGCondition *) {
                assert(false);
                std::cerr << "No accept for EGCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(AGCondition *) {
                assert(false);
                std::cerr << "No accept for AGCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(AFCondition *) {
                assert(false);
                std::cerr << "No accept for AFCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(EXCondition *) {
                assert(false);
                std::cerr << "No accept for EXCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(AXCondition *) {
                assert(false);
                std::cerr << "No accept for AXCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(EUCondition *) {
                assert(false);
                std::cerr << "No accept for EUCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(AUCondition *) {
                assert(false);
                std::cerr << "No accept for AUCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(ACondition *) {
                assert(false);
                std::cerr << "No accept for ACondition" << std::endl;
                exit(0);
            };

            virtual void _accept(ECondition *) {
                assert(false);
                std::cerr << "No accept for ECondition" << std::endl;
                exit(0);
            };

            virtual void _accept(GCondition *) {
                assert(false);
                std::cerr << "No accept for GCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(FCondition *) {
                assert(false);
                std::cerr << "No accept for FCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(XCondition *) {
                assert(false);
                std::cerr << "No accept for XCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(UntilCondition *) {
                assert(false);
                std::cerr << "No accept for UntilCondition" << std::endl;
                exit(0);
            };

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(UnfoldedFireableCondition *element) {
                assert(false);
                std::cerr << "No accept for UnfoldedFireableCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(FireableCondition *element) {
                assert(false);
                std::cerr << "No accept for FireableCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(UpperBoundsCondition *element) {
                assert(false);
                std::cerr << "No accept for UpperBoundsCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(LivenessCondition *element) {
                assert(false);
                std::cerr << "No accept for LivenessCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(KSafeCondition *element) {
                assert(false);
                std::cerr << "No accept for KSafeCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(QuasiLivenessCondition *element) {
                assert(false);
                std::cerr << "No accept for QuasiLivenessCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(StableMarkingCondition *element) {
                assert(false);
                std::cerr << "No accept for StableMarkingCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(BooleanCondition *element) {
                assert(false);
                std::cerr << "No accept for BooleanCondition" << std::endl;
                exit(0);
            };

            // Expression
            virtual void _accept(UnfoldedIdentifierExpr *element) {
                assert(false);
                std::cerr << "No accept for UnfoldedIndentifierExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(LiteralExpr *element) {
                assert(false);
                std::cerr << "No accept for LiteralExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(PlusExpr *element) {
                assert(false);
                std::cerr << "No accept for PlusExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(MultiplyExpr *element) {
                assert(false);
                std::cerr << "No accept for MultiplyExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(MinusExpr *element) {
                assert(false);
                std::cerr << "No accept for MinusExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(SubtractExpr *element) {
                assert(false);
                std::cerr << "No accept for SubtractExpr" << std::endl;
                exit(0);
            };

            // shallow expression, default to error
            virtual void _accept(IdentifierExpr *element) {
                assert(false);
                std::cerr << "No accept for IdentifierExpr" << std::endl;
                exit(0);
            };
        };
    }
}
#endif //VERIFYPN_MUTATINGVISITOR_H
