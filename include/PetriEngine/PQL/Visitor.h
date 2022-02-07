/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   Visitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 11, 2020, 1:07 PM
 */

#ifndef VISITOR_H
#define VISITOR_H

#include "PetriEngine/PQL/Expressions.h"
#include "utils/errors.h"
#include <type_traits>

namespace PetriEngine {
    namespace PQL {
        class Visitor {
        public:
            Visitor() {}

            template<typename T>
            void accept(T &&element) {
                _accept(element);
            }

            //virtual ~Visitor() = default;

        protected:
            virtual void _accept(const NotCondition* element) = 0;

            virtual void _accept(const AndCondition* element) {
                element->LogicalCondition::visit(*this);
            }

            virtual void _accept(const OrCondition* element) {
                element->LogicalCondition::visit(*this);
            }

            virtual void _accept(const LessThanCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(const LessThanOrEqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(const EqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(const NotEqualCondition* element) {
                element->CompareCondition::visit(*this);
            }

            virtual void _accept(const DeadlockCondition* element) = 0;
            virtual void _accept(const CompareConjunction* element) = 0;
            virtual void _accept(const UnfoldedUpperBoundsCondition* element) = 0;

            // Super classes, the default implementation of subclasses is to call these
            virtual void _accept(const CommutativeExpr *element) {
                element->NaryExpr::visit(*this);
            }

            virtual void _accept(const SimpleQuantifierCondition *element) {
                assert(false);
                throw base_error("No accept for SimpleQuantifierCondition (may be called from subclass)");
            }

            virtual void _accept(const LogicalCondition *element) {
                assert(false);
                throw base_error("No accept for LogicalCondition (may be called from subclass)");
            }

            virtual void _accept(const CompareCondition *element) {
                assert(false);
                throw base_error("No accept for CompareCondition (may be called from subclass)");
            }

            virtual void _accept(const UntilCondition *element) {
                assert(false);
                throw base_error("No accept for UntilCondition (may be called from subclass)");
            }


            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(const ControlCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const ACondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const ECondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const GCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const FCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const XCondition *condition) {
                condition->SimpleQuantifierCondition::visit(*this);
            };

            virtual void _accept(const ShallowCondition *element) {
                if (element->getCompiled()) {
                    element->getCompiled()->visit(*this);
                } else {
                    assert(false);
                    throw base_error("No accept for ShallowCondition");
                }
            }

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(const UnfoldedFireableCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const FireableCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const UpperBoundsCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const LivenessCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const KSafeCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const QuasiLivenessCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const StableMarkingCondition *element) {
                element->ShallowCondition::visit(*this);
            };

            virtual void _accept(const BooleanCondition *element) {
                assert(false);
                throw base_error("No accept for BooleanCondition");
            };

            // Expression
            virtual void _accept(const UnfoldedIdentifierExpr *element) {
                assert(false);
                throw base_error("No accept for UnfoldedIdentifierExpr");
            };

            virtual void _accept(const LiteralExpr *element) {
                assert(false);
                throw base_error("No accept for LiteralExpr");
            };

            virtual void _accept(const PlusExpr *element) {
                element->CommutativeExpr::visit(*this);
            };

            virtual void _accept(const MultiplyExpr *element) {
                element->CommutativeExpr::visit(*this);
            };

            virtual void _accept(const MinusExpr *element) {
                assert(false);
                throw base_error("No accept for MinusExpr");
            };

            virtual void _accept(const NaryExpr *element) {
                assert(false);
                throw base_error("No accept for LivenessCondition");
            }

            virtual void _accept(const SubtractExpr *element) {
                element->NaryExpr::visit(*this);
            }

            // shallow expression, default to error
            virtual void _accept(const IdentifierExpr *element) {
                assert(false);
                throw base_error("No accept for IdentifierExpr");
            };
        };

        class ExpressionVisitor : public Visitor {
        public:

        private:
            void _accept(const NotCondition *element) override {
                assert(false);
                throw base_error("No accept for NotCondition");
            };

            void _accept(const AndCondition *element) override {
                assert(false);
                throw base_error("No accept for AndCondition");
            };

            void _accept(const OrCondition *element) override {
                assert(false);
                throw base_error("No accept for OrCondition");
            };

            void _accept(const LessThanCondition *element) override {
                assert(false);
                throw base_error("No accept for LessThanCondition");
            };

            void _accept(const LessThanOrEqualCondition *element) override {
                assert(false);
                throw base_error("No accept for LessThanOrEqualCondition");
            };

            void _accept(const EqualCondition *element) override {
                assert(false);
                throw base_error("No accept for EqualCondition");
            };

            void _accept(const NotEqualCondition *element) override {
                assert(false);
                throw base_error("No accept for NotEqualCondition");
            };

            void _accept(const DeadlockCondition *element) override {
                assert(false);
                throw base_error("No accept for DeadlockCondition");
            };

            void _accept(const CompareConjunction *element) override {
                assert(false);
                throw base_error("No accept for CompareConjunction");
            };

            void _accept(const UnfoldedUpperBoundsCondition *element) override {
                assert(false);
                throw base_error("No accept for UnfoldedUpperBoundsCondition");
            };
        };

        class BaseVisitor : public Visitor {
        protected:
            void _accept(const NotCondition *element) override
            {
                element->getCond()->visit(*this);
            }

            void _accept(const LogicalCondition *element) override
            {
                for (const auto &cond : *element) {
                    cond->visit(*this);
                }
            }

            void _accept(const CompareCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const DeadlockCondition *element) override
            {
                // no-op
            }

            void _accept(const CompareConjunction *element) override
            {
                // no-op, complicated
            }

            void _accept(const UnfoldedUpperBoundsCondition *element) override
            {
                // no-op
            }

            void _accept(const SimpleQuantifierCondition *condition) override {
                condition->getCond()->visit(*this);
            }

            void _accept(const UntilCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const ShallowCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const BooleanCondition *element) override
            {
                // no-op
            }

            void _accept(const UnfoldedIdentifierExpr *element) override
            {
                // no-op
            }

            void _accept(const LiteralExpr *element) override
            {
                // no-op
            }

            void _accept(const NaryExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const MinusExpr *element) override
            {
                (*element)[0]->visit(*this);
            }

            void _accept(const IdentifierExpr *element) override
            {
                if(const auto& compiled = element->compiled())
                    compiled->visit(*this);
                // no-op
            }
        };

        // Used to make visitors that check if any node in the tree fulfills a condition
        class AnyVisitor : public BaseVisitor {
        public:
            [[nodiscard]] bool getReturnValue() const { return _condition_found; }

        protected:
            bool _condition_found = false;
            void setConditionFound() { _condition_found = true; }

        private:
        };
    }
}

#endif /* VISITOR_H */

