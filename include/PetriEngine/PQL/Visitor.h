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
            virtual void _accept(const AndCondition* element) = 0;
            virtual void _accept(const OrCondition* element) = 0;
            virtual void _accept(const LessThanCondition* element) = 0;
            virtual void _accept(const LessThanOrEqualCondition* element) = 0;
            virtual void _accept(const EqualCondition* element) = 0;
            virtual void _accept(const NotEqualCondition* element) = 0;

            virtual void _accept(const DeadlockCondition* element) = 0;
            virtual void _accept(const CompareConjunction* element) = 0;
            virtual void _accept(const UnfoldedUpperBoundsCondition* element) = 0;

            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(const EFCondition *) {
                assert(false);
                std::cerr << "No accept for EFCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const EGCondition *) {
                assert(false);
                std::cerr << "No accept for EGCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const AGCondition *) {
                assert(false);
                std::cerr << "No accept for AGCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const AFCondition *) {
                assert(false);
                std::cerr << "No accept for AFCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const EXCondition *) {
                assert(false);
                std::cerr << "No accept for EXCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const AXCondition *) {
                assert(false);
                std::cerr << "No accept for AXCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const EUCondition *) {
                assert(false);
                std::cerr << "No accept for EUCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const AUCondition *) {
                assert(false);
                std::cerr << "No accept for AUCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const ACondition *) {
                assert(false);
                std::cerr << "No accept for ACondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const ECondition *) {
                assert(false);
                std::cerr << "No accept for ECondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const GCondition *) {
                assert(false);
                std::cerr << "No accept for GCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const FCondition *) {
                assert(false);
                std::cerr << "No accept for FCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const XCondition *) {
                assert(false);
                std::cerr << "No accept for XCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const UntilCondition *) {
                assert(false);
                std::cerr << "No accept for UntilCondition" << std::endl;
                exit(0);
            };

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(const UnfoldedFireableCondition *element) {
                assert(false);
                std::cerr << "No accept for UnfoldedFireableCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const FireableCondition *element) {
                assert(false);
                std::cerr << "No accept for FireableCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const UpperBoundsCondition *element) {
                assert(false);
                std::cerr << "No accept for UpperBoundsCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const LivenessCondition *element) {
                assert(false);
                std::cerr << "No accept for LivenessCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const KSafeCondition *element) {
                assert(false);
                std::cerr << "No accept for KSafeCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const QuasiLivenessCondition *element) {
                assert(false);
                std::cerr << "No accept for QuasiLivenessCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const StableMarkingCondition *element) {
                assert(false);
                std::cerr << "No accept for StableMarkingCondition" << std::endl;
                exit(0);
            };

            virtual void _accept(const BooleanCondition *element) {
                assert(false);
                std::cerr << "No accept for BooleanCondition" << std::endl;
                exit(0);
            };

            // Expression
            virtual void _accept(const UnfoldedIdentifierExpr *element) = 0;

            virtual void _accept(const LiteralExpr *element) = 0;

            virtual void _accept(const PlusExpr *element) = 0;

            virtual void _accept(const MultiplyExpr *element) = 0;

            virtual void _accept(const MinusExpr *element) = 0;

            virtual void _accept(const SubtractExpr *element) = 0;

            // shallow expression, default to error
            virtual void _accept(const IdentifierExpr *element) {
                assert(false);
                std::cerr << "No accept for IdentifierExpr" << std::endl;
                exit(0);
            };
        };

        class ExpressionVisitor : public Visitor {
        public:

        private:
            void _accept(const NotCondition *element) override {
                assert(false);
                std::cerr << "No accept for NotCondition" << std::endl;
                exit(0);
            };

            void _accept(const AndCondition *element) override {
                assert(false);
                std::cerr << "No accept for AndCondition" << std::endl;
                exit(0);
            };

            void _accept(const OrCondition *element) override {
                assert(false);
                std::cerr << "No accept for OrCondition" << std::endl;
                exit(0);
            };

            void _accept(const LessThanCondition *element) override {
                assert(false);
                std::cerr << "No accept for LessThanCondition" << std::endl;
                exit(0);
            };

            void _accept(const LessThanOrEqualCondition *element) override {
                assert(false);
                std::cerr << "No accept for LessThanOrEqualCondition" << std::endl;
                exit(0);
            };

            void _accept(const EqualCondition *element) override {
                assert(false);
                std::cerr << "No accept for EqualCondition" << std::endl;
                exit(0);
            };

            void _accept(const NotEqualCondition *element) override {
                assert(false);
                std::cerr << "No accept for NotEqualCondition" << std::endl;
                exit(0);
            };

            void _accept(const DeadlockCondition *element) override {
                assert(false);
                std::cerr << "No accept for DeadlockCondition" << std::endl;
                exit(0);
            };

            void _accept(const CompareConjunction *element) override {
                assert(false);
                std::cerr << "No accept for CompareConjunction" << std::endl;
                exit(0);
            };

            void _accept(const UnfoldedUpperBoundsCondition *element) override {
                assert(false);
                std::cerr << "No accept for UnfoldedUpperBoundsCondition" << std::endl;
                exit(0);
            };
        };

        class BaseVisitor : public Visitor {
        protected:
            void _accept(const NotCondition *element) override
            {
                (*element)[0]->visit(*this);
            }

            void _accept(const AndCondition *element) override
            {
                for (const auto &cond : *element) {
                    cond->visit(*this);
                }
            }

            void _accept(const OrCondition *element) override
            {
                for (const auto &cond : *element) {
                    cond->visit(*this);
                }
            }

            void _accept(const LessThanCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const LessThanOrEqualCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const EqualCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const NotEqualCondition *element) override
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

            void _accept(const EFCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EGCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AGCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AFCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EXCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AXCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EUCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const AUCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const ACondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const ECondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const GCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const FCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const XCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const UntilCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const UnfoldedFireableCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const FireableCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const UpperBoundsCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const LivenessCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const KSafeCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const QuasiLivenessCondition *element) override
            {
                if (const auto &compiled = element->getCompiled())
                    compiled->visit(*this);
            }

            void _accept(const StableMarkingCondition *element) override
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

            void _accept(const PlusExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const MultiplyExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const MinusExpr *element) override
            {
                (*element)[0]->visit(*this);
            }

            void _accept(const SubtractExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const IdentifierExpr *element) override
            {
                // no-op
            }
        };
    }
}

#endif /* VISITOR_H */

