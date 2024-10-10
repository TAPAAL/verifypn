#include "PetriEngine/ExplicitColored/GuardExpression.h"
#include <string>
namespace PetriEngine {
    namespace ExplicitColored { 

        class ColorSeqEvaluationVisitor : public Colored::ColorExpressionVisitor {
        public:
            ColorSeqEvaluationVisitor(const Colored::ColorTypeMap& colorTypeMap, const Binding& binding)
                :_colorTypeMap(&colorTypeMap), _binding(&binding) 
            {
            }

            void accept(const Colored::TupleExpression* expr) override {
                std::vector<Color_t> finalColors;
                for (const auto& colorExpression : *expr) {
                    colorExpression->visit(*this);
                    if (colorTypeSequence.size() != 1) {
                        throw base_error("Product colors cannot be nested");
                    }
                    finalColors.push_back(colorTypeSequence[0]);
                    colorTypeSequence.clear();
                }
                colorTypeSequence = std::move(finalColors);
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
                if (colorTypeSequence.size() != 1) {
                    throw base_error("Cannot get successor of product color");
                }
                auto ct = expr->getColorType(*_colorTypeMap);
                const Colored::Color& color = (*ct)[colorTypeSequence[0]];
                colorTypeSequence[0] = color.operator++().getId();
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                if (colorTypeSequence.size() != 1) {
                    throw base_error("Cannot get successor of product color");
                }
                auto ct = expr->getColorType(*_colorTypeMap);
                const Colored::Color& color = (*ct)[colorTypeSequence[0]];
                colorTypeSequence[0] = color.operator--().getId();
            }

            void accept(const Colored::VariableExpression* expr) override {
                colorTypeSequence.push_back(std::hash<std::string>{}(expr->variable()->name));
            }

            void accept(const Colored::DotConstantExpression*) override {}
            void accept(const Colored::UserOperatorExpression*) override {}
            void accept(const Colored::LessThanExpression*) override {}
            void accept(const Colored::LessThanEqExpression*) override {}
            void accept(const Colored::EqualityExpression*) override {}
            void accept(const Colored::InequalityExpression*) override {}
            void accept(const Colored::AndExpression*) override {}
            void accept(const Colored::OrExpression*) override {}
            void accept(const Colored::AllExpression*) override {}
            void accept(const Colored::NumberOfExpression*) override {}
            void accept(const Colored::AddExpression*) override {}
            void accept(const Colored::SubtractExpression*) override {}
            void accept(const Colored::ScalarProductExpression*) override {}

            std::vector<Color_t> colorTypeSequence;
        private:
            const Colored::ColorTypeMap* const _colorTypeMap;
            const Binding* const _binding;
            Color_t _currentColor;
        };

        class BooleanEvaluationVisitor : public Colored::ColorExpressionVisitor {
        public:
            BooleanEvaluationVisitor(const Colored::ColorTypeMap& colorTypeMap, const Binding& binding)
                : _colorTypeMap(&colorTypeMap), _binding(&binding), _sequenceVisitor(colorTypeMap, binding) {
                evaluation = false;
            }

            void accept(const Colored::LessThanExpression* expr) override {
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                if (left.size() != 1 && right.size() != 1) {
                    throw base_error("Cannot do ordered comparison of non basic color types");
                }
                evaluation = left[0] < right[0];    
            }

            void accept(const Colored::LessThanEqExpression* expr) override {
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                if (left.size() != 1 && right.size() != 1) {
                    throw base_error("Cannot do ordered comparison of non basic color types");
                }
                evaluation = left[0] <= right[0];  
            }

            void accept(const Colored::EqualityExpression* expr) override {
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                if (left.size() != right.size()) {
                    evaluation = false;
                    return;
                }

                for (size_t i = 0; i < left.size(); i++) {
                    if (left[i] != right[i]) {
                        evaluation = false;
                        return;
                    }
                }
                evaluation = true;
            }

            void accept(const Colored::InequalityExpression* expr) override {
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                if (left.size() != right.size()) {
                    evaluation = true;
                    return;
                }

                for (size_t i = 0; i < left.size(); i++) {
                    if (left[i] != right[i]) {
                        evaluation = true;
                        return;
                    }
                }
                evaluation = false;
            }

            void accept(const Colored::AndExpression* expr) override {
                (*expr)[0]->visit(*this);
                if (!evaluation)
                    return;
                (*expr)[1]->visit(*this);
            }

            void accept(const Colored::OrExpression* expr) override {
                (*expr)[0]->visit(*this);
                if (evaluation)
                    return;
                (*expr)[1]->visit(*this);
            }

            void accept(const Colored::DotConstantExpression*) override {}
            void accept(const Colored::VariableExpression*) override {}
            void accept(const Colored::UserOperatorExpression*) override {}
            void accept(const Colored::SuccessorExpression*) override {}
            void accept(const Colored::PredecessorExpression*) override {}
            void accept(const Colored::TupleExpression*) override {}
            void accept(const Colored::AllExpression*) override {}
            void accept(const Colored::NumberOfExpression*) override {}
            void accept(const Colored::AddExpression*) override {}
            void accept(const Colored::SubtractExpression*) override {}
            void accept(const Colored::ScalarProductExpression*) override {}

            bool evaluation;
        private:
            const Colored::ColorTypeMap* const _colorTypeMap;
            const Binding* const _binding;
            ColorSeqEvaluationVisitor _sequenceVisitor;
        };

        GuardExpression::GuardExpression(std::shared_ptr<Colored::ColorTypeMap> colorTypeMap, Colored::GuardExpression_ptr guardExpression)
            : _colorTypeMap(std::move(colorTypeMap)),
            _guardExpression(std::move(guardExpression)) {}
        
        bool eval(const Binding &binding) {
            return false;
        }
    }
}

