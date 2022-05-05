/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/CloningVisitor.h"

namespace PetriEngine {
    namespace Colored {

        void CloningVisitor::accept(const DotConstantExpression *expression) {
            _col_res = std::make_shared<PetriEngine::Colored::DotConstantExpression>();
        }

        void CloningVisitor::accept(const VariableExpression *expression) {
            _col_res = std::make_shared<PetriEngine::Colored::VariableExpression>(expression->variable());
        }

        void CloningVisitor::accept(const UserOperatorExpression *expression) {
            _col_res = std::make_shared<PetriEngine::Colored::UserOperatorExpression>(expression->user_operator());
        }

        void CloningVisitor::accept(const SuccessorExpression *expression) {
            expression->child()->visit(*this);
            _col_res = std::make_shared<PetriEngine::Colored::SuccessorExpression>(std::move(_col_res));
        }

        void CloningVisitor::accept(const PredecessorExpression *expression) {
            expression->child()->visit(*this);
            _col_res = std::make_shared<PetriEngine::Colored::PredecessorExpression>(std::move(_col_res));
        }

        void CloningVisitor::accept(const TupleExpression *expression) {
            std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
            for (auto &expr : *expression) {
                expr->visit(*this);
                colors.emplace_back(std::move(_col_res));
            }
            _col_res = std::make_shared<PetriEngine::Colored::TupleExpression>(std::move(colors), expression->colorType());
        }

        void CloningVisitor::accept(const LessThanExpression *expression) {
            expression[0].visit(*this);
            auto left = _col_res;
            expression[1].visit(*this);
            auto right = _col_res;
            _guard_res = std::make_shared<PetriEngine::Colored::LessThanExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const LessThanEqExpression *expression) {
            expression[0].visit(*this);
            auto left = _col_res;
            expression[1].visit(*this);
            auto right = _col_res;
            _guard_res = std::make_shared<PetriEngine::Colored::LessThanEqExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const EqualityExpression *expression) {
            expression[0].visit(*this);
            auto left = _col_res;
            expression[1].visit(*this);
            auto right = _col_res;
            _guard_res = std::make_shared<PetriEngine::Colored::EqualityExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const InequalityExpression *expression) {
            expression[0].visit(*this);
            auto left = _col_res;
            expression[1].visit(*this);
            auto right = _col_res;
            _guard_res = std::make_shared<PetriEngine::Colored::InequalityExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const AndExpression *expression) {
            expression[0].visit(*this);
            auto left = _guard_res;
            expression[1].visit(*this);
            auto right = _guard_res;
            _guard_res = std::make_shared<PetriEngine::Colored::AndExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const OrExpression *expression) {
            expression[0].visit(*this);
            auto left = _guard_res;
            expression[1].visit(*this);
            auto right = _guard_res;
            _guard_res = std::make_shared<PetriEngine::Colored::OrExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const AllExpression *expression) {
            _all_res = std::make_shared<AllExpression>(expression->sort());
        }

        void CloningVisitor::accept(const NumberOfExpression *expression) {
            if (expression->is_all()) {
                expression->all()->visit(*this);
                _arc_res = std::make_shared<NumberOfExpression>(std::move(_all_res), expression->number());
            } else {
                std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
                for (auto &expr : *expression) {
                    expr->visit(*this);
                    colors.emplace_back(std::move(_col_res));
                }
                _arc_res = std::make_shared<NumberOfExpression>(std::move(colors), expression->number());
            }
        }

        void CloningVisitor::accept(const AddExpression *expression) {
            std::vector<ArcExpression_ptr> constituents;
            for (auto& expr : *expression) {
                expr->visit(*this);
                constituents.emplace_back(std::move(_arc_res));
            }
            _arc_res = std::make_shared<PetriEngine::Colored::AddExpression>(std::move(constituents));
        }

        void CloningVisitor::accept(const SubtractExpression *expression) {
            expression[0].visit(*this);
            auto left = _arc_res;
            expression[1].visit(*this);
            auto right = _arc_res;
            _arc_res = std::make_shared<PetriEngine::Colored::SubtractExpression>(std::move(left), std::move(right));
        }

        void CloningVisitor::accept(const ScalarProductExpression *expression) {
            expression->child()->visit(*this);
            _arc_res = std::make_shared<ScalarProductExpression>(std::move(_arc_res), expression->scalar());
        }
    }
}