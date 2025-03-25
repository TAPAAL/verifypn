//
// Created by emil on 12/4/24.
//

#ifndef GAMMAQUERYVISITOR_H
#define GAMMAQUERYVISITOR_H
#include <PetriEngine/ExplicitColored/FireabilityChecker.h>
#include <PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h>
#include <PetriEngine/PQL/Visitor.h>
#include "ColoredExpressionVisitor.h"

namespace PetriEngine {
    namespace ExplicitColored {
        class GammaQueryVisitor : public PQL::Visitor {
        public:
            static bool eval(
                const PQL::Condition_ptr& expr,
                const ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
                const ColoredSuccessorGenerator& successorGenerator
            ) {
                GammaQueryVisitor visitor{marking, placeNameIndices, transitionNameIndices, successorGenerator};
                visit(visitor, expr);

                return visitor._answer;
            }
        protected:
            explicit GammaQueryVisitor(
                const ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
                const ColoredSuccessorGenerator& successorGenerator
            )
                : _marking(marking), _successorGenerator(successorGenerator), _placeNameIndices(placeNameIndices), _transitionNameIndices(transitionNameIndices) { }

            void _accept(const PQL::NotCondition *element) override {
                visit(this, element->getCond().get());
                _answer = !_answer;
            }

            void _accept(const PQL::AndCondition *expr) override {
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    if (!_answer) {
                        return;
                    }
                }
            }

            void _accept(const PQL::OrCondition *expr) override {
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    if (_answer) {
                        return;
                    }
                }
            }

            void _accept(const PQL::LessThanCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs < rhs;
            }

            void _accept(const PQL::LessThanOrEqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs <= rhs;
            }

            void _accept(const PQL::EqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs == rhs;
            }

            void _accept(const PQL::NotEqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs != rhs;
            }

            void _accept(const PQL::DeadlockCondition *element) override {
                _answer = FireabilityChecker::hasDeadlock(_successorGenerator, _marking);
            }

            void _accept(const PQL::FireableCondition *element) override {
                _answer = FireabilityChecker::canFire(_successorGenerator, _transitionNameIndices.find(*element->getName())->second, _marking);
            }

            void _accept(const PQL::EFCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PQL::AGCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PQL::LiteralExpr *element) override {
                invalid();
            }

            void _accept(const PQL::IdentifierExpr *element) override {
                invalid();
            }

            void _accept(const PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
            void _accept(const PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
            void _accept(const PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
            void _accept(const PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
            void _accept(const PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
            void _accept(const PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
            void _accept(const PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
            void _accept(const PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
            void _accept(const PQL::PathQuant *element) override  { notSupported("PathQuant"); }
            void _accept(const PQL::ExistPath *element) override  { notSupported("ExistPath"); }
            void _accept(const PQL::AllPaths *element) override  { notSupported("AllPaths"); }
            void _accept(const PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
            void _accept(const PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
            void _accept(const PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
            void _accept(const PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
            void _accept(const PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
            void _accept(const PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
            void _accept(const PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
            void _accept(const PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
            void _accept(const PQL::ACondition *condition) override  { notSupported("ACondition"); }
            void _accept(const PQL::ECondition *condition) override  { notSupported("ECondition"); }
            void _accept(const PQL::GCondition *condition) override  { notSupported("GCondition"); }
            void _accept(const PQL::FCondition *condition) override  { notSupported("FCondition"); }
            void _accept(const PQL::XCondition *condition) override  { notSupported("XCondition"); }
            void _accept(const PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
            void _accept(const PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
            void _accept(const PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
            void _accept(const PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
            void _accept(const PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
            void _accept(const PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
            void _accept(const PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
            void _accept(const PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
            void _accept(const PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
            void _accept(const PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
            void _accept(const PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
            void _accept(const PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
            void _accept(const PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
            void _accept(const PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
        private:
            void notSupported() {
                throw base_error("Not supported");
            }
            void notSupported(const std::string& type) {
                throw base_error("Not supported ", type);
            }

            void invalid() {
                throw base_error("Invalid expression");
            }

            bool _answer = true;
            const ColoredPetriNetMarking& _marking;
            const ColoredSuccessorGenerator& _successorGenerator;
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
            const std::unordered_map<std::string, uint32_t>& _transitionNameIndices;
        };
    }
}
#endif //GAMMAQUERYVISITOR_H
