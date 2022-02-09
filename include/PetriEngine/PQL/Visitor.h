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

            Visitor() {
            }

            template<typename T>
            void accept(T &&element) {
                _accept(element);
            }

            template<typename T> struct is_smart_ptr : std::false_type {};
            template<typename T> struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};
            template<typename T> struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
            template<typename T> struct is_smart_ptr<const std::shared_ptr<T>> : std::true_type {};
            template<typename T> struct is_smart_ptr<const std::unique_ptr<T>> : std::true_type {};


            template<typename T, typename Q>
            static void visit(T& visitor, Q* c) {
                if constexpr (is_smart_ptr<T>::value)
                    visit(visitor.get(), c);
                else
                    visit(&visitor, c);
            }

            template<typename T, typename Q>
            static void visit(T& visitor, Q& c) {
                if constexpr (is_smart_ptr<T>::value)
                    visit(visitor.get(), c);
                else
                    visit(&visitor, c);
            }

            template<typename T, typename Q>
            static void visit(T* visitor, Q& c) {
                if constexpr (is_smart_ptr<Q>::value)
                    visit(visitor, c.get());
                else
                    visit(visitor, &c);
            }

            template<typename T, typename Q>
            static void visit(T* visitor, const Q* c) {
                visit(visitor, const_cast<Q*> (c));
            }

            template<typename T, typename Q>
            static void visit_expr(T* visitor, Q* c) {
                if constexpr (type_id<Q>() != untyped)
                {
                    visitor->accept(c);
                }
                else
                {
                    switch (c->type()) {
                        case type_id<PlusExpr>():
                            visitor->accept(static_cast<PlusExpr*> (c));
                            break;
                        case type_id<MinusExpr>():
                            visitor->accept(static_cast<MinusExpr*> (c));
                            break;
                        case type_id<SubtractExpr>():
                            visitor->accept(static_cast<SubtractExpr*> (c));
                            break;
                        case type_id<MultiplyExpr>():
                            visitor->accept(static_cast<MultiplyExpr*> (c));
                            break;
                        case type_id<IdentifierExpr>():
                            visitor->accept(static_cast<IdentifierExpr*> (c));
                            break;
                        case type_id<LiteralExpr>():
                            visitor->accept(static_cast<LiteralExpr*> (c));
                            break;
                        case type_id<UnfoldedIdentifierExpr>():
                            visitor->accept(static_cast<UnfoldedIdentifierExpr*> (c));
                            break;
                        default:
                            __builtin_unreachable(); // <-- helps the compiler optimize
                    }
                }
            }

            template<typename T, typename Q>
            static void visit(T* visitor, Q* c) {
                if constexpr (std::is_base_of<Expr,Q>::value)
                        visit_expr(visitor, c);
                else
                {
                    if constexpr (type_id<Q>() != untyped)
                    {
                        visitor->accept(c);
                    }
                    else
                    {
                        switch (c->type()) {
                            case type_id<OrCondition>():
                                visitor->accept(static_cast<OrCondition*> (c));
                                break;
                            case type_id<AndCondition>():
                                visitor->accept(static_cast<AndCondition*> (c));
                                break;
                            case type_id<CompareConjunction>():
                                visitor->accept(static_cast<CompareConjunction*> (c));
                                break;
                            case type_id<LessThanCondition>():
                                visitor->accept(static_cast<LessThanCondition*> (c));
                                break;
                            case type_id<LessThanOrEqualCondition>():
                                visitor->accept(static_cast<LessThanOrEqualCondition*> (c));
                                break;
                            case type_id<EqualCondition>():
                                visitor->accept(static_cast<EqualCondition*> (c));
                                break;
                            case type_id<NotEqualCondition>():
                                visitor->accept(static_cast<NotEqualCondition*> (c));
                                break;
                            case type_id<DeadlockCondition>():
                                visitor->accept(static_cast<DeadlockCondition*> (c));
                                break;
                            case type_id<UnfoldedUpperBoundsCondition>():
                                visitor->accept(static_cast<UnfoldedUpperBoundsCondition*> (c));
                                break;
                            case type_id<NotCondition>():
                                visitor->accept(static_cast<NotCondition*> (c));
                                break;
                            case type_id<BooleanCondition>():
                                visitor->accept(static_cast<BooleanCondition*> (c));
                                break;
                            case type_id<ECondition>():
                                visitor->accept(static_cast<ECondition*> (c));
                                break;
                            case type_id<ACondition>():
                                visitor->accept(static_cast<ACondition*> (c));
                                break;
                            case type_id<FCondition>():
                                visitor->accept(static_cast<FCondition*> (c));
                                break;
                            case type_id<GCondition>():
                                visitor->accept(static_cast<GCondition*> (c));
                                break;
                            case type_id<UntilCondition>():
                                visitor->accept(static_cast<UntilCondition*> (c));
                                break;
                            case type_id<XCondition>():
                                visitor->accept(static_cast<XCondition*> (c));
                                break;
                            case type_id<ControlCondition>():
                                visitor->accept(static_cast<ControlCondition*> (c));
                                break;
                            case type_id<StableMarkingCondition>():
                                visitor->accept(static_cast<StableMarkingCondition*> (c));
                                break;
                            case type_id<QuasiLivenessCondition>():
                                visitor->accept(static_cast<QuasiLivenessCondition*> (c));
                                break;
                            case type_id<LivenessCondition>():
                                visitor->accept(static_cast<LivenessCondition*> (c));
                                break;
                            case type_id<KSafeCondition>():
                                visitor->accept(static_cast<KSafeCondition*> (c));
                                break;
                            case type_id<UpperBoundsCondition>():
                                visitor->accept(static_cast<UpperBoundsCondition*> (c));
                                break;
                            case type_id<FireableCondition>():
                                visitor->accept(static_cast<FireableCondition*> (c));
                                break;
                            case type_id<UnfoldedFireableCondition>():
                                visitor->accept(static_cast<UnfoldedFireableCondition*> (c));
                                break;
                            default:
                                __builtin_unreachable(); // <-- helps the compiler optimize
                        }
                    }
                }
            }

            //virtual ~Visitor() = default;

        protected:

            virtual void _accept(const Condition* element) {
                assert(false);
                throw base_error("No accept for Condition (may be called from subclass)");
            }

            virtual void _accept(const NotCondition* element) {
                _accept(static_cast<const Condition*>(element));
            };

            virtual void _accept(const AndCondition* element) {
                _accept(static_cast<const LogicalCondition*> (element));
            }

            virtual void _accept(const OrCondition* element) {
                _accept(static_cast<const LogicalCondition*> (element));
            }

            virtual void _accept(const LessThanCondition* element) {
                _accept(static_cast<const CompareCondition*> (element));
            }

            virtual void _accept(const LessThanOrEqualCondition* element) {
                _accept(static_cast<const CompareCondition*> (element));
            }

            virtual void _accept(const EqualCondition* element) {
                _accept(static_cast<const CompareCondition*> (element));
            }

            virtual void _accept(const NotEqualCondition* element) {
                _accept(static_cast<const CompareCondition*> (element));
            }

            virtual void _accept(const DeadlockCondition* element) {
                _accept(static_cast<const Condition*>(element));
            };

            virtual void _accept(const CompareConjunction* element) {
                _accept(static_cast<const Condition*>(element));
            };

            virtual void _accept(const UnfoldedUpperBoundsCondition* element) {
                _accept(static_cast<const Condition*>(element));
            };

            // Super classes, the default implementation of subclasses is to call these

            virtual void _accept(const CommutativeExpr *element) {
                _accept(static_cast<const NaryExpr*> (element));
            }

            virtual void _accept(const SimpleQuantifierCondition *element) {
                _accept(static_cast<const QuantifierCondition*>(element));
            }

            virtual void _accept(const LogicalCondition *element) {
                _accept(static_cast<const Condition*>(element));
            }

            virtual void _accept(const CompareCondition *element) {
                _accept(static_cast<const Condition*>(element));
            }

            virtual void _accept(const UntilCondition *element) {
                _accept(static_cast<const Condition*>(element));
            }


            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error

            virtual void _accept(const ControlCondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const ACondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const ECondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const GCondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const FCondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const XCondition *condition) {
                _accept(static_cast<const SimpleQuantifierCondition*> (condition));
            };

            virtual void _accept(const ShallowCondition *element) {
                if (element->getCompiled()) {
                    visit(this, element->getCompiled());
                } else {
                    _accept(static_cast<const Condition*>(element));
                }
            }

            // shallow elements, neither of these should exist in a compiled expression

            virtual void _accept(const UnfoldedFireableCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const FireableCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const UpperBoundsCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const LivenessCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const KSafeCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const QuasiLivenessCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const StableMarkingCondition *element) {
                _accept(static_cast<const ShallowCondition*> (element));
            };

            virtual void _accept(const BooleanCondition *element) {
                _accept(static_cast<const Condition*>(element));
            };

            // Expression
            virtual void _accept(const Expr *element) {
                assert(false);
                throw base_error("No accept for Expr (may be called from subclass");
            }

            virtual void _accept(const UnfoldedIdentifierExpr *element) {
                _accept(static_cast<const Expr*>(element));
            };

            virtual void _accept(const LiteralExpr *element) {
                _accept(static_cast<const Expr*>(element));
            };

            virtual void _accept(const PlusExpr *element) {
                _accept(static_cast<const CommutativeExpr*> (element));
            };

            virtual void _accept(const MultiplyExpr *element) {
                _accept(static_cast<const CommutativeExpr*> (element));
            };

            virtual void _accept(const MinusExpr *element) {
                _accept(static_cast<const Expr*>(element));
            };

            virtual void _accept(const NaryExpr *element) {
                _accept(static_cast<const Expr*>(element));
            }

            virtual void _accept(const SubtractExpr *element) {
                _accept(static_cast<const NaryExpr*> (element));
            }

            // shallow expression, default to error

            virtual void _accept(const IdentifierExpr *element) {
                if(element->compiled())
                    Visitor::visit(this, element->compiled());
                else
                {
                    _accept(static_cast<const Expr*>(element));
                }
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

            void _accept(const NotCondition *element) override {
                visit(this, element->getCond());
            }

            void _accept(const LogicalCondition *element) override {
                for (const auto &cond : *element) {
                    visit(this, cond);
                }
            }

            void _accept(const CompareCondition *element) override {
                visit(this, element->getExpr1());
                visit(this, element->getExpr2());
            }

            void _accept(const DeadlockCondition *element) override {
                // no-op
            }

            void _accept(const CompareConjunction *element) override {
                // no-op, complicated
            }

            void _accept(const UnfoldedUpperBoundsCondition *element) override {
                // no-op
            }

            void _accept(const SimpleQuantifierCondition *condition) override {
                visit(this, condition->getCond());
            }

            void _accept(const UntilCondition *condition) override {
                visit(this, (*condition)[0]);
                visit(this, (*condition)[1]);
            }

            void _accept(const ShallowCondition *element) override {
                if (const auto &compiled = element->getCompiled())
                    visit(this, compiled);
            }

            void _accept(const BooleanCondition *element) override {
                // no-op
            }

            void _accept(const UnfoldedIdentifierExpr *element) override {
                // no-op
            }

            void _accept(const LiteralExpr *element) override {
                // no-op
            }

            void _accept(const NaryExpr *element) override {
                for (const auto &expr : element->expressions()) {
                    visit(this, expr);
                }
            }

            void _accept(const MinusExpr *element) override {
                visit(this, (*element)[0]);
            }

            void _accept(const IdentifierExpr *element) override {
                if (const auto& compiled = element->compiled())
                    visit(this, compiled);
                // no-op
            }
        };

        // Used to make visitors that check if any node in the tree fulfills a condition

        class AnyVisitor : public BaseVisitor {
        public:
            [[nodiscard]] bool getReturnValue() const {
                return _condition_found;
            }

        protected:
            bool _condition_found = false;

            void setConditionFound() {
                _condition_found = true;
            }

        private:
        };
    }
}

#endif /* VISITOR_H */

