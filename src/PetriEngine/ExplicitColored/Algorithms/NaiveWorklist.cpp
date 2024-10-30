#ifndef NAIVEWORKLIST_CPP
#define NAIVEWORKLIST_CPP

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

        void _accept(const PetriEngine::PQL::NotCondition *element) override { notSupported("NotCondition"); }
        void _accept(const PetriEngine::PQL::AndCondition *element) override { notSupported("AndCondition"); }
        void _accept(const PetriEngine::PQL::OrCondition *element) override { notSupported("OrCondition"); }
        void _accept(const PetriEngine::PQL::LessThanCondition *element) override { notSupported("LessThanCondition"); }
        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override { notSupported("LessThanOrEqualCondition"); }
        void _accept(const PetriEngine::PQL::EqualCondition *element) override { notSupported("EqualCondition"); }
        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override { notSupported("NotEqualCondition"); }
        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override { notSupported("DeadlockCondition"); }
        void _accept(const PetriEngine::PQL::EFCondition *condition) override { notSupported("EFCondition"); }
        void _accept(const PetriEngine::PQL::FireableCondition *element) override { notSupported("FireableCondition"); }
        void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
        void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
        void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
        void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
        void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
        void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
        void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
        void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported("PathQuant"); }
        void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported("ExistPath"); }
        void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported("AllPaths"); }
        void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
        void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
        void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
        void _accept(const PetriEngine::PQL::AGCondition *condition) override  { notSupported("AGCondition"); }
        void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
        void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
        void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
        void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
        void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
        void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported("ACondition"); }
        void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported("ECondition"); }
        void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported("GCondition"); }
        void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported("FCondition"); }
        void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported("XCondition"); }
        void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
        void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
        void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
        void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
        void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
        void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
        void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
        void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
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
        PetriEngine::ExplicitColored::MarkingCount_t _evaluated;
        const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
    };

    class ColoredQueryVisitor : public PetriEngine::PQL::Visitor {
    public:
        static CheckStatus eval(
            const PetriEngine::PQL::Condition_ptr& expr,
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            DeadlockValue deadlockValue
        ) {
            ColoredQueryVisitor visitor{marking, placeNameIndices, deadlockValue};
            
            if (auto cond = dynamic_cast<PetriEngine::PQL::EFCondition*>(expr.get())) {
                visit(visitor, cond->getCond());
                if (visitor._dependsOnDeadlock) {
                    return CheckStatus::UNSATISFIED_CONTINUE;
                } else {
                    return visitor._answer ? CheckStatus::SATISIFIED : CheckStatus::UNSATISFIED_CONTINUE;
                }
            } else if (auto cond = dynamic_cast<PetriEngine::PQL::AGCondition*>(expr.get())) {
                visit(visitor, cond->getCond());
                if (visitor._dependsOnDeadlock) {
                    return CheckStatus::SATISFIED_CONTINUE;
                } else {
                    return visitor._answer ? CheckStatus::SATISFIED_CONTINUE : CheckStatus::UNSATISFIED;
                }
            } else {
                visit(visitor, expr);
                if (visitor._dependsOnDeadlock) {
                    return CheckStatus::UNSATISFIED_CONTINUE;
                }
                return visitor._answer ? CheckStatus::SATISIFIED : CheckStatus::UNSATISFIED;
            }
        }
    protected:
        explicit ColoredQueryVisitor(
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            DeadlockValue deadlockValue
        )
            : _answer(true), _marking(marking), _placeNameIndices(placeNameIndices), _deadlockValue(deadlockValue) { }

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
            if (_deadlockValue == DeadlockValue::TRUE) {
                _answer = true;
            } else if (_deadlockValue == DeadlockValue::FALSE) {
                _answer = false;
            } else {
                _dependsOnDeadlock = true;
            }
        }

        void _accept(const PetriEngine::PQL::FireableCondition *element) override {
            notSupported("Does not support fireable");
        }

        void _accept(const PetriEngine::PQL::EFCondition *condition) override {
            notSupported("Does not supported nested quantifiers");
        }
        
        void _accept(const PetriEngine::PQL::AGCondition *condition) override {
            notSupported("Does not supported nested quantifiers");
        }

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override {
            invalid();
        }

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {
            invalid();
        }

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
        void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
        void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
        void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
        void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
        void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
        void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
        void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported("PathQuant"); }
        void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported("ExistPath"); }
        void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported("AllPaths"); }
        void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
        void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
        void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
        void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
        void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
        void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
        void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
        void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
        void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported("ACondition"); }
        void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported("ECondition"); }
        void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported("GCondition"); }
        void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported("FCondition"); }
        void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported("XCondition"); }
        void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
        void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
        void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
        void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
        void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
        void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
        void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
        void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
        void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
        void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
        void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
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

        bool _answer;
        bool _dependsOnDeadlock = false;
        DeadlockValue _deadlockValue;
        const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
    };

    bool NaiveWorklist::check(){
        auto gen = PetriEngine::ExplicitColored::ColoredSuccessorGenerator(_net);
        return bfs(gen, _net.initial());
    }

    template<typename S>
    CheckStatus NaiveWorklist::check(S state, DeadlockValue deadlockValue) {
        return ColoredQueryVisitor::eval(_formula, state, _placeNameIndices, deadlockValue);
    }

    template<typename S>
    bool NaiveWorklist::bfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, const S& state){
        auto waiting = light_deque<S>{64};
        auto passed = PetriEngine::ExplicitColored::ColoredMarkingSet {};
        waiting.push_back(state);
        passed.add(state);
        auto lastCheck = check(state, DeadlockValue::UNKNOWN);
        if (lastCheck == CheckStatus::SATISIFIED || lastCheck == CheckStatus::UNSATISFIED){
            return lastCheck == CheckStatus::SATISIFIED;
        }
        while (!waiting.empty()){
            auto next = waiting.front();
            waiting.pop_front();

            auto successors = successor_generator.next(next);

            if (successors.empty()) {
                lastCheck = check(next, DeadlockValue::TRUE);
            } else {
                lastCheck = check(next, DeadlockValue::FALSE);
            }
            if (lastCheck == CheckStatus::SATISIFIED || lastCheck == CheckStatus::UNSATISFIED){
                std::cout << "Checked " << passed.size() << " states" << std::endl;
                return lastCheck == CheckStatus::SATISIFIED;
            }
            for (;!successors.empty(); successors = successor_generator.next(next)) {
                for (auto&& s : successors){
                    if (!passed.contains(s)) {
                        lastCheck = check(state, DeadlockValue::UNKNOWN);
                        if (lastCheck == CheckStatus::SATISIFIED || lastCheck == CheckStatus::UNSATISFIED){
                            std::cout << "Checked " << passed.size() << " states" << std::endl;
                            return lastCheck == CheckStatus::SATISIFIED;
                        }
                        passed.add(s);
                        waiting.push_back(s);
                    }
                }
            }

            successor_generator.reset();
        }
        std::cout << "Checked " << passed.size() << " states" << std::endl;
        return lastCheck == CheckStatus::SATISFIED_CONTINUE;
    }

    template<typename S>
    bool NaiveWorklist::dfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, S& state){
        auto waiting = std::queue<S>{state};
        auto passed = PetriEngine::ExplicitColored::ColoredMarkingSet {};
        if (_formula(state)){
            return true;
        }
        while (!waiting.empty()){
            auto next = waiting.pop();

            while(true){
                auto successors = successor_generator.next(next);
                if (successors.empty()){
                    successor_generator.reset();
                    break;
                }
                for (auto&& s : successors){
                    if (!passed.contains(s)) {
                        if (check(s)){
                            std::cout << "Checked " << passed.size() << " states" << std::endl;
                            return true;
                        }
                        passed.add(s);
                        waiting.push_back(s);
                    }
                }
            }
        }
        return false;
    }
}

#endif //NAIVEWORKLIST_CPP