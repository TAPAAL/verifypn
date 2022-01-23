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

            virtual void _accept(NotCondition* element) = 0;

            virtual void _accept(AndCondition* element) {
                element->LogicalCondition::visit(*this);
            }

            virtual void _accept(OrCondition* element) {
                element->LogicalCondition::visit(*this);
            }

            virtual void _accept(LessThanCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(LessThanOrEqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(EqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(NotEqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(DeadlockCondition* element) = 0;
            virtual void _accept(CompareConjunction* element) = 0;
            virtual void _accept(UnfoldedUpperBoundsCondition* element) = 0;

            // Super classes, the default implementation of subclasses is to call these
            virtual void _accept(CommutativeExpr *element) {
                element->NaryExpr::visit(*this);
            }

            virtual void _accept(SimpleQuantifierCondition *element) {
                assert(false);
                std::cerr << "No accept for SimpleQuantifierCondition (may be called from subclass)" << std::endl;
                exit(0);
            }

            virtual void _accept(LogicalCondition *element) {
                assert(false);
                std::cerr << "No accept for LogicalCondition (may be called from subclass)" << std::endl;
                exit(0);
            }

            virtual void _accept(CompareCondition *element) {
                assert(false);
                std::cerr << "No accept for CompareCondition (may be called from subclass)" << std::endl;
                exit(0);
            }

            virtual void _accept(UntilCondition *element) {
                assert(false);
                std::cerr << "No accept for UntilCondition (may be called from subclass)" << std::endl;
                exit(0);
            }


            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(ControlCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(EFCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(EGCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(AGCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(AFCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(EXCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(AXCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(EUCondition *condition) {
                condition->UntilCondition::visit(*this);
            };

            virtual void _accept(AUCondition *condition) {
                condition->UntilCondition::visit(*this);
            };

            virtual void _accept(ACondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(ECondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(GCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(FCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(XCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(ShallowCondition *element) {
                if (element->getCompiled()) {
                    element->getCompiled()->visit(*this);
                } else {
                    assert(false);
                    std::cerr << "No accept for ShallowCondition" << std::endl;
                    exit(0);
                }
            }

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(UnfoldedFireableCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(FireableCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(UpperBoundsCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(LivenessCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(KSafeCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(QuasiLivenessCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(StableMarkingCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(BooleanCondition *element) {
                assert(false);
                std::cerr << "No accept for BooleanCondition" << std::endl;
                exit(0);
            };

            // Expression
            virtual void _accept(UnfoldedIdentifierExpr *element) {
                assert(false);
                std::cerr << "No accept for UnfoldedIdentifierExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(LiteralExpr *element) {
                assert(false);
                std::cerr << "No accept for LiteralExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(PlusExpr *element) {
                element->CommutativeExpr::visit(*this);
            };

            virtual void _accept(MultiplyExpr *element) {
                element->CommutativeExpr::visit(*this);
            };

            virtual void _accept(MinusExpr *element) {
                assert(false);
                std::cerr << "No accept for MinusExpr" << std::endl;
                exit(0);
            };

            virtual void _accept(NaryExpr *element) {
                assert(false);
                std::cerr << "No accept for LivenessCondition" << std::endl;
                exit(0);
            }

            virtual void _accept(SubtractExpr *element) {
                element->NaryExpr::visit(*this);
            }

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
