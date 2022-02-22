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
#include "Visitor.h"
#include "utils/errors.h"

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
            virtual void _accept(Condition* element) {
                assert(false);
                throw base_error("No accept for Condition (may be called from subclass)");
            }

            virtual void _accept(NotCondition* element) {
                _accept(static_cast<Condition*>(element));
            };

            virtual void _accept(AndCondition* element) {
                _accept(static_cast<LogicalCondition*>(element));
            }

            virtual void _accept(OrCondition* element) {
                _accept(static_cast<LogicalCondition*>(element));
            }

            virtual void _accept(LessThanCondition* element) {
                _accept(static_cast<CompareCondition*>(element));
            }

            virtual void _accept(LessThanOrEqualCondition* element) {
                _accept(static_cast<CompareCondition*>(element));
            }

            virtual void _accept(EqualCondition* element) {
                _accept(static_cast<CompareCondition*>(element));
            }

            virtual void _accept(NotEqualCondition* element) {
                _accept(static_cast<CompareCondition*>(element));
            }

            virtual void _accept(DeadlockCondition* element) {
                _accept(static_cast<Condition*>(element));
            }

            virtual void _accept(CompareConjunction* element) {
                _accept(static_cast<Condition*>(element));
            }

            virtual void _accept(UnfoldedUpperBoundsCondition* element) {
                _accept(static_cast<Condition*>(element));
            }

            // Super classes, the default implementation of subclasses is to call these
            virtual void _accept(CommutativeExpr *element) {
                _accept(static_cast<NaryExpr*>(element));
            }

            virtual void _accept(SimpleQuantifierCondition *element) {
                _accept(static_cast<QuantifierCondition*>(element));
            }

            virtual void _accept(LogicalCondition *element) {
                _accept(static_cast<Condition*>(element));
            }

            virtual void _accept(CompareCondition *element) {
                _accept(static_cast<Condition*>(element));
            }

            virtual void _accept(UntilCondition *element) {
                _accept(static_cast<QuantifierCondition*>(element));
            }

            virtual void _accept(ReleaseCondition *element) {
                _accept(static_cast<QuantifierCondition*>(element));
            }

            virtual void _accept(ControlCondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(ACondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(ECondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(GCondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(FCondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(XCondition *condition) {
                _accept(static_cast<SimpleQuantifierCondition*>(condition));
            };

            virtual void _accept(ShallowCondition *element) {
                if (element->getCompiled()) {
                    Visitor::visit(this, element->getCompiled());
                } else {
                    throw base_error("No accept for ShallowCondition");
                }
            }

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(UnfoldedFireableCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(FireableCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(UpperBoundsCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(LivenessCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(KSafeCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(QuasiLivenessCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(StableMarkingCondition *element) {
                _accept(static_cast<ShallowCondition*>(element));
            };

            virtual void _accept(BooleanCondition *element) {
                _accept(static_cast<Condition*>(element));
            };

            // Expression
            virtual void _accept(UnfoldedIdentifierExpr *element) {
                _accept(static_cast<Expr*>(element));
            };

            virtual void _accept(Expr *element) {
                assert(false);
                throw base_error("No accept for Expr (May be called from derived class)");
            }

            virtual void _accept(LiteralExpr *element) {
                _accept(static_cast<Expr*>(element));
            };

            virtual void _accept(PlusExpr *element) {
                _accept(static_cast<CommutativeExpr*>(element));
            };

            virtual void _accept(MultiplyExpr *element) {
                _accept(static_cast<CommutativeExpr*>(element));
            };

            virtual void _accept(MinusExpr *element) {
                _accept(static_cast<Expr*>(element));
            };

            virtual void _accept(NaryExpr *element) {
                _accept(static_cast<Expr*>(element));
            };

            virtual void _accept(SubtractExpr *element) {
                _accept(static_cast<NaryExpr*>(element));
            }

            virtual void _accept(IdentifierExpr *element) {
                if (element->compiled()) {
                    Visitor::visit(*this, element->compiled());
                } else {
                    _accept(static_cast<Expr*>(element));
                }
            };
        };
    }
}
#endif //VERIFYPN_MUTATINGVISITOR_H
