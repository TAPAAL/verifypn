#ifndef CONDITION_COPY_VISITOR_H
#define CONDITION_COPY_VISITOR_H
#include <PetriEngine/PQL/Visitor.h>

namespace PetriEngine::ExplicitColored {
    class ConditionCopyVisitor final : public PQL::Visitor {
    public:
        static PQL::Condition_ptr copyCondition(const PQL::Condition_ptr& condition) {
            ConditionCopyVisitor visitor;
            visit(visitor, condition);
            return visitor._copiedCondition;
        }
    protected:
        void _accept(const PQL::NotCondition *element) override {
             visit(this, element->getCond());
            _copiedCondition = std::make_shared<PQL::NotCondition>(_copiedCondition);
        }

        void _accept(const PQL::AndCondition *element) override {
            std::vector<PQL::Condition_ptr> conditionCopies;
            for (const auto& condition : *element) {
                visit(this, condition);
                conditionCopies.push_back(std::move(_copiedCondition));
            }
            _copiedCondition = std::make_shared<PQL::AndCondition>(std::move(conditionCopies));
        }

        void _accept(const PQL::OrCondition *element) override {
            std::vector<PQL::Condition_ptr> conditionCopies;
            for (const auto& condition : *element) {
                visit(this, condition);
                conditionCopies.push_back(std::move(_copiedCondition));
            }
            _copiedCondition = std::make_shared<PQL::OrCondition>(std::move(conditionCopies));
        }

        void _accept(const PQL::LessThanCondition *element) override {
            visit(this, element->getExpr1());
            auto lhs = _copiedExpr;
            visit(this, element->getExpr2());
            _copiedCondition = std::make_shared<PQL::LessThanCondition>(lhs, _copiedExpr);
        }

        void _accept(const PQL::LessThanOrEqualCondition *element) override {
            visit(this, element->getExpr1());
            auto lhs = _copiedExpr;
            visit(this, element->getExpr2());
            _copiedCondition = std::make_shared<PQL::LessThanOrEqualCondition>(lhs, _copiedExpr);
        }

        void _accept(const PQL::EqualCondition *element) override {
            visit(this, element->getExpr1());
            auto lhs = _copiedExpr;
            visit(this, element->getExpr2());
            _copiedCondition = std::make_shared<PQL::EqualCondition>(lhs, _copiedExpr);
        }

        void _accept(const PQL::NotEqualCondition *element) override {
            visit(this, element->getExpr1());
            auto lhs = _copiedExpr;
            visit(this, element->getExpr2());
            _copiedCondition = std::make_shared<PQL::NotEqualCondition>(lhs, _copiedExpr);
        }

        void _accept(const PQL::DeadlockCondition *element) override {
            _copiedCondition = PQL::DeadlockCondition::DEADLOCK;
        }

        void _accept(const PQL::CompareConjunction *element) override {
            _copiedCondition = std::make_shared<PQL::CompareConjunction>(*element);
        }

        void _accept(const PQL::UnfoldedUpperBoundsCondition *element) override {
            _copiedCondition = std::make_shared<PQL::UnfoldedUpperBoundsCondition>(*element);
        }

        void _accept(const PQL::CommutativeExpr *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::SimpleQuantifierCondition *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::LogicalCondition *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::UntilCondition *element) override {
            visit(this, element->getCond1());
            auto lhs = _copiedCondition;
            visit(this, element->getCond2());
            _copiedCondition = std::make_shared<PQL::UntilCondition>(lhs, _copiedCondition);
        }

        void _accept(const PQL::ControlCondition *condition) override {
            _copiedCondition = std::make_shared<PQL::ControlCondition>(*condition);
        }

        void _accept(const PQL::PathQuant *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::ExistPath *element) override {
            _copiedCondition = std::make_shared<PQL::ExistPath>(element->name(), element->child(), element->offset());
        }

        void _accept(const PQL::AllPaths *element) override {
            _copiedCondition = std::make_shared<PQL::AllPaths>(element->name(), element->child(), element->offset());
        }

        void _accept(const PQL::PathSelectCondition *element) override {
            _copiedCondition = std::make_shared<PQL::PathSelectCondition>(element->name(), element->child(), element->offset());
        }

        void _accept(const PQL::PathSelectExpr *element) override {
            visit(this, element->child());
            _copiedExpr = std::make_shared<PQL::PathSelectExpr>(element->name(), _copiedExpr, element->offset());
        }

        void _accept(const PQL::EFCondition *condition) override {
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::EFCondition>(_copiedCondition);
        }

        void _accept(const PQL::EGCondition *condition) override {
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::EGCondition>(_copiedCondition);
        }

        void _accept(const PQL::AGCondition *condition) override {
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::AGCondition>(_copiedCondition);
        }

        void _accept(const PQL::AFCondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::AFCondition>(_copiedCondition);
        }

        void _accept(const PQL::EXCondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::EXCondition>(_copiedCondition);
        }

        void _accept(const PQL::AXCondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::AXCondition>(_copiedCondition);
        }

        void _accept(const PQL::EUCondition *condition) override {
            visit(this, condition->getCond1());
            auto lhs = _copiedCondition;
            visit(this, condition->getCond2());
            _copiedCondition = std::make_shared<PQL::EUCondition>(lhs, _copiedCondition);
        }

        void _accept(const PQL::AUCondition *condition) override {
            visit(this, condition->getCond1());
            auto lhs = _copiedCondition;
            visit(this, condition->getCond2());
            _copiedCondition = std::make_shared<PQL::AUCondition>(lhs, _copiedCondition);

        }

        void _accept(const PQL::ACondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::ACondition>(_copiedCondition);
        }

        void _accept(const PQL::ECondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::ECondition>(_copiedCondition);
        }

        void _accept(const PQL::GCondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::GCondition>(_copiedCondition);
        }

        void _accept(const PQL::FCondition *condition) override {
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::FCondition>(_copiedCondition);
        }

        void _accept(const PQL::XCondition *condition) override{
            visit(this, condition->getCond());
            _copiedCondition = std::make_shared<PQL::XCondition>(_copiedCondition);
        }

        void _accept(const PQL::ShallowCondition *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::UnfoldedFireableCondition *element) override {
            _copiedCondition = std::make_shared<PQL::UnfoldedFireableCondition>(element->getName());
        }

        void _accept(const PQL::FireableCondition *element) override {
            _copiedCondition = std::make_shared<PQL::FireableCondition>(element->getName());
        }

        void _accept(const PQL::UpperBoundsCondition *element) override {
            _copiedCondition = std::make_shared<PQL::UpperBoundsCondition>(element->getPlaces());
        }

        void _accept(const PQL::LivenessCondition *element) override {
            _copiedCondition = std::make_shared<PQL::LivenessCondition>();
        }

        void _accept(const PQL::KSafeCondition *element) override {
            visit(this, element->getBound());
            _copiedCondition = std::make_shared<PQL::KSafeCondition>(_copiedExpr);
        }

        void _accept(const PQL::QuasiLivenessCondition *element) override {
            _copiedCondition = std::make_shared<PQL::QuasiLivenessCondition>();
        }

        void _accept(const PQL::StableMarkingCondition *element) override {
            _copiedCondition = std::make_shared<PQL::StableMarkingCondition>();
        }

        void _accept(const PQL::BooleanCondition *element) override {
            _copiedCondition = std::make_shared<PQL::BooleanCondition>(element->value);
        }

        void _accept(const PQL::CompareCondition *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::UnfoldedIdentifierExpr *element) override {
            _copiedExpr = std::make_shared<PQL::UnfoldedIdentifierExpr>(element->name(), element->offset());
        }

        void _accept(const PQL::LiteralExpr *element) override {
            _copiedExpr = std::make_shared<PQL::LiteralExpr>(element->value());
        }

        void _accept(const PQL::PlusExpr *element) override {
            std::vector<PQL::Expr_ptr> copiedExpressions;
            for (const auto& expr : element->expressions()) {
                visit(this, expr);
                copiedExpressions.push_back(_copiedExpr);
            }
            _copiedExpr = std::make_shared<PQL::PlusExpr>(std::move(copiedExpressions));
        }

        void _accept(const PQL::MultiplyExpr *element) override {
            std::vector<PQL::Expr_ptr> copiedExpressions;
            for (const auto& expr : element->expressions()) {
                visit(this, expr);
                copiedExpressions.push_back(_copiedExpr);
            }
            _copiedExpr = std::make_shared<PQL::MultiplyExpr>(std::move(copiedExpressions));
        }

        void _accept(const PQL::MinusExpr *element) override {
            visit(this, element);
            _copiedExpr = std::make_shared<PQL::MinusExpr>(_copiedExpr);
        }

        void _accept(const PQL::NaryExpr *element) override {
            throw base_error("Cannot copy abstract class");
        }

        void _accept(const PQL::SubtractExpr *element) override {
            std::vector<PQL::Expr_ptr> copiedExpressions;
            for (const auto& expr : element->expressions()) {
                visit(this, expr);
                copiedExpressions.push_back(_copiedExpr);
            }
            _copiedExpr = std::make_shared<PQL::SubtractExpr>(std::move(copiedExpressions));
        }

        void _accept(const PQL::IdentifierExpr *element) override {
            _copiedExpr = std::make_shared<PQL::IdentifierExpr>(element->name());
        }

    private:
        ConditionCopyVisitor() {}
        PQL::Condition_ptr _copiedCondition;
        PQL::Expr_ptr _copiedExpr;
    };

}

#endif