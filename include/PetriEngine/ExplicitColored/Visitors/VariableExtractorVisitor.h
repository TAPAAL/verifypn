#ifndef VARIABLEEXTRACTORVSITOR_H
#define VARIABLEEXTRACTORVSITOR_H

#include "../../Colored/ColorExpressionVisitor.h"
#include "../../Colored/Expressions.h"
#include "../AtomicTypes.h"
#include <unordered_map>
#include <set>
#include <string>

namespace PetriEngine::ExplicitColored {
    class VariableExtractorVisitor final : public Colored::ColorExpressionVisitor {
    public:
        explicit VariableExtractorVisitor(const std::unordered_map<std::string, Variable_t>& variableMap)
            : _variableMap(&variableMap) {
        }

        void accept(const Colored::TupleExpression* expr) override {
            for (const auto& subExpr : *expr) {
                subExpr->visit(*this);
            }
        }

        void accept(const Colored::SuccessorExpression* expr) override {
            expr->child()->visit(*this);
        }

        void accept(const Colored::PredecessorExpression* expr) override {
            expr->child()->visit(*this);
        }

        void accept(const Colored::VariableExpression* expr) override {
            collectedVariables.insert(_variableMap->find(expr->variable()->name)->second);
        }

        void accept(const Colored::UserOperatorExpression* expr) override {
        }

        void accept(const Colored::DotConstantExpression* expr) override {
        }

        void accept(const Colored::LessThanExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::LessThanEqExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::EqualityExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::InequalityExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::AndExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::OrExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::AllExpression*) override {
        }

        void accept(const Colored::NumberOfExpression* expr) override {
            for (const auto& subExpr : *expr) {
                subExpr->visit(*this);
            }
        }

        void accept(const Colored::AddExpression* expr) override {
            for (const auto& subExpr : *expr) {
                subExpr->visit(*this);
            }
        }

        void accept(const Colored::SubtractExpression* expr) override {
            (*expr)[0]->visit(*this);
            (*expr)[1]->visit(*this);
        }

        void accept(const Colored::ScalarProductExpression* expr) override {
            expr->visit(*this);
        }

        std::set<Variable_t> collectedVariables;
        const std::unordered_map<std::string, Variable_t>* const _variableMap;
    };
}

#endif //VARIABLEEXTRACTORVSITOR_H
