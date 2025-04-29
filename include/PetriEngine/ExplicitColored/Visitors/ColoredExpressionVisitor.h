#ifndef COLOREDEXPRESSIONVISITOR_H
#define COLOREDEXPRESSIONVISITOR_H
#include <PetriEngine/ExplicitColored/ColoredPetriNetMarking.h>
#include <PetriEngine/PQL/Expressions.h>
#include <PetriEngine/PQL/Visitor.h>

namespace PetriEngine {
    namespace ExplicitColored {
        class ColoredExpressionEvaluator : public PQL::Visitor {
        public:
            static MarkingCount_t eval(
                const PQL::Expr_ptr& expr,
                const ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices
            ) {
                ColoredExpressionEvaluator visitor{marking, placeNameIndices};
                visit(visitor, expr.get());
                return visitor._evaluated;
            }
        protected:
            explicit ColoredExpressionEvaluator(
                const ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices
                ) : _placeNameIndices(placeNameIndices), _evaluated(0), _marking(marking) {}

            void _accept(const PQL::LiteralExpr *element) override {
                _evaluated = static_cast<MarkingCount_t>(element->value());
            }

            void _accept(const PQL::IdentifierExpr *element) override {
                auto placeIndexIt = _placeNameIndices.find(*(element->name()));
                if (placeIndexIt == _placeNameIndices.end()) {
                    throw base_error("Unknown place in query ", *(element->name()));
                }
                _evaluated = _marking.markings[placeIndexIt->second].totalCount();
            }

            void _accept(const PQL::NotCondition *element) override { notSupported("NotCondition"); }
            void _accept(const PQL::AndCondition *element) override { notSupported("AndCondition"); }
            void _accept(const PQL::OrCondition *element) override { notSupported("OrCondition"); }
            void _accept(const PQL::LessThanCondition *element) override { notSupported("LessThanCondition"); }
            void _accept(const PQL::LessThanOrEqualCondition *element) override { notSupported("LessThanOrEqualCondition"); }
            void _accept(const PQL::EqualCondition *element) override { notSupported("EqualCondition"); }
            void _accept(const PQL::NotEqualCondition *element) override { notSupported("NotEqualCondition"); }
            void _accept(const PQL::DeadlockCondition *element) override { notSupported("DeadlockCondition"); }
            void _accept(const PQL::EFCondition *condition) override { notSupported("EFCondition"); }
            void _accept(const PQL::FireableCondition *element) override { notSupported("FireableCondition"); }
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
            void _accept(const PQL::AGCondition *condition) override  { notSupported("AGCondition"); }
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
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
            MarkingCount_t _evaluated;
            const ColoredPetriNetMarking& _marking;


            static void notSupported() {
                throw base_error("Not supported");
            }

            static void notSupported(const std::string& type) {
                throw base_error("Not supported ", type);
            }

            static void invalid() {
                throw base_error("Invalid expression");
            }
        };
    }
}
#endif //COLOREDEXPRESSIONVISITOR_H
