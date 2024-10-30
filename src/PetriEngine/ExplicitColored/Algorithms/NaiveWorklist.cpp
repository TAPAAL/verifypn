#ifndef NAIVEWORKLIST_CPP
#define NAIVEWORKLIST_CPP

#include <limits>
#include "PetriEngine/ExplicitColored/Algorithms/NaiveWorklist.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredMarkingSet.h"
namespace ColoredLTL {
    class ColoredExpressionEvaluator : public PetriEngine::PQL::Visitor {
    public:
        static PetriEngine::ExplicitColored::MarkingCount_t eval(
            const PetriEngine::PQL::Expr_ptr& expr,
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
        ) {
            ColoredExpressionEvaluator visitor{marking, placeNameIndices};
            visit(visitor, expr.get());
            return visitor._evaluated;
        }
    protected:
        explicit ColoredExpressionEvaluator(
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
            ) : _placeNameIndices(placeNameIndices), _evaluated(0), _marking(marking) {}

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override {
            _evaluated = static_cast<PetriEngine::ExplicitColored::MarkingCount_t>(element->value());
        }

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {
            auto placeIndexIt = _placeNameIndices.find(*(element->name()));
            if (placeIndexIt == _placeNameIndices.end()) {
                throw base_error("Unknown place in query ", *(element->name()));
            }
            _evaluated = _marking.markings[placeIndexIt->second].totalCount();
        }

        void _accept(const PetriEngine::PQL::NotCondition *element) override { }
        void _accept(const PetriEngine::PQL::AndCondition *element) override { }
        void _accept(const PetriEngine::PQL::OrCondition *element) override { }
        void _accept(const PetriEngine::PQL::LessThanCondition *element) override { }
        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override { }
        void _accept(const PetriEngine::PQL::EqualCondition *element) override { }
        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override { }
        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override { }
        void _accept(const PetriEngine::PQL::EFCondition *condition) override { }
        void _accept(const PetriEngine::PQL::FireableCondition *element) override { }
        void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported(); }
        void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AGCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported(); }
        void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported(); }
    private:
        void notSupported() {
            throw base_error("Not supported");
        }

        void invalid() {
            throw base_error("Invalid expression");
        }
        PetriEngine::ExplicitColored::MarkingCount_t _evaluated;
        const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
    };

    class ColoredQueryVisitor : public PetriEngine::PQL::Visitor {
    public:
        static PetriEngine::ExplicitColored::MarkingCount_t eval(
            const PetriEngine::PQL::Condition_ptr& expr,
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
        ) {
            ColoredQueryVisitor visitor{marking, placeNameIndices};
            visit(visitor, expr.get());
            return visitor._answer;
        }
    protected:
        explicit ColoredQueryVisitor(
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
        )
            : _answer(true), _marking(marking), _placeNameIndices(placeNameIndices) { }

        void _accept(const PetriEngine::PQL::NotCondition *element) override {
            visit(this, element->getCond().get());
            _answer = !_answer;
        }

        void _accept(const PetriEngine::PQL::AndCondition *expr) override {
            for (const auto& subExpr : *expr) {
                visit(this, subExpr.get());
                if (!_answer) {
                    return;
                }
            }
        }

        void _accept(const PetriEngine::PQL::OrCondition *expr) override {
            for (const auto& subExpr : *expr) {
                visit(this, subExpr.get());
                if (_answer) {
                    return;
                }
            }
        }

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override {
            const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            _answer = lhs < rhs;
        }

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override {
            const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            _answer = lhs <= rhs;
        }

        void _accept(const PetriEngine::PQL::EqualCondition *element) override {
            const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            _answer = lhs == rhs;
        }

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override {
            const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            _answer = lhs != rhs;
        }

        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override {
            throw base_error("Unsupported");
        }

        void _accept(const PetriEngine::PQL::EFCondition *condition) override {
            visit(this, condition->getCond().get());
        }

        void _accept(const PetriEngine::PQL::FireableCondition *element) override {
            throw base_error("Unsupported");
        }

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override {
            invalid();
        }

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {
            invalid();
        }

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported(); }
        void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AGCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported(); }
        void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported(); }
        void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported(); }
    private:
        void notSupported() {
            throw base_error("Not supported");
        }

        void invalid() {
            throw base_error("Invalid expression");
        }

        bool _answer;
        const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
    };

    bool NaiveWorklist::check(){
        auto gen = PetriEngine::ExplicitColored::ColoredSuccessorGenerator(_net);
        return bfs(gen, _net.initial());
    }

    template<typename S>
    bool NaiveWorklist::check(S state) {
        return ColoredQueryVisitor::eval(_formula, state, _placeNameIndices);
    }

    template<typename S>
    bool NaiveWorklist::bfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, const S& state){
        auto waiting = light_deque<PetriEngine::ExplicitColored::ColoredPetriNetState>{64};
        auto passed = PetriEngine::ExplicitColored::ColoredMarkingSet {};
        waiting.push_back(PetriEngine::ExplicitColored::ColoredPetriNetState{state});
        passed.add(state);
        if (check(state)){
            std::cout << "Inital state satisfies the formula " << std::endl;
            return true;
        }
        while (!waiting.empty()){
            auto& next = waiting.front();
            auto successor = successor_generator.next(next);
            if (successor.lastTrans ==  std::numeric_limits<uint32_t>::max()){
                waiting.pop_front();
                continue;
            }
            auto& marking = successor.marking;
            if (!passed.contains(marking)){
                if (check(marking)){
                    std::cout << "Checked " << passed.size() << " states" << std::endl;
                    return true;
                }
                passed.add(marking);
                waiting.push_back(std::move(successor));
            }
        }
        std::cout << "Checked " << passed.size() << " states" << std::endl;
        return false;
    }
}
#endif //NAIVEWORKLIST_CPP