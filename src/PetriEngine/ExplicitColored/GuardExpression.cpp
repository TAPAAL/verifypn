#include "PetriEngine/ExplicitColored/GuardExpression.h"
#include <string>
namespace PetriEngine {
    namespace ExplicitColored { 

        class ColorSeqEvaluationVisitor : public Colored::ColorExpressionVisitor {
        public:
            ColorSeqEvaluationVisitor(const Colored::ColorTypeMap& colorTypeMap, const Binding& binding, const std::unordered_map<std::string, Variable_t>& variable_map)
                :_colorTypeMap(&colorTypeMap), _binding(&binding), _variableMap(&variable_map)
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
                const auto test = _variableMap->find(expr->variable()->name);
                if (test == _variableMap->end())
                    throw base_error("Unknown variable");
                colorTypeSequence.push_back(_binding->getValue(test->second));
            }

            void accept(const Colored::DotConstantExpression*) override {unexpectedExpression();}
            void accept(const Colored::UserOperatorExpression*) override {unexpectedExpression();}
            void accept(const Colored::LessThanExpression*) override {unexpectedExpression();}
            void accept(const Colored::LessThanEqExpression*) override {unexpectedExpression();}
            void accept(const Colored::EqualityExpression*) override {unexpectedExpression();}
            void accept(const Colored::InequalityExpression*) override {unexpectedExpression();}
            void accept(const Colored::AndExpression*) override {unexpectedExpression();}
            void accept(const Colored::OrExpression*) override {unexpectedExpression();}
            void accept(const Colored::AllExpression*) override {unexpectedExpression();}
            void accept(const Colored::NumberOfExpression*) override {unexpectedExpression();}
            void accept(const Colored::AddExpression*) override {unexpectedExpression();}
            void accept(const Colored::SubtractExpression*) override {unexpectedExpression();}
            void accept(const Colored::ScalarProductExpression*) override {unexpectedExpression();}

            std::vector<Color_t> colorTypeSequence;

            void reset() {
                colorTypeSequence.clear();
            }
        private:
            const Colored::ColorTypeMap* const _colorTypeMap;
            const Binding* const _binding;
            Color_t _currentColor;
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            
            void unexpectedExpression() {
                throw base_error("unexpected exception");
            }
        };

        class BooleanEvaluationVisitor : public Colored::ColorExpressionVisitor {
        public:
            BooleanEvaluationVisitor(const Colored::ColorTypeMap& colorTypeMap, const Binding& binding, const std::unordered_map<std::string, Variable_t>& variableMap)
                : _colorTypeMap(&colorTypeMap),
                _binding(&binding),
                _sequenceVisitor(colorTypeMap, binding, variableMap),
                _variableMap(&variableMap) {
                evaluation = false;
            }

            void accept(const Colored::LessThanExpression* expr) override {
                _sequenceVisitor.reset();
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                
                _sequenceVisitor.reset();
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                
                if (left.size() != 1 && right.size() != 1) {
                    throw base_error("Cannot do ordered comparison of non basic color types");
                }
                evaluation = left[0] < right[0];    
            }

            void accept(const Colored::LessThanEqExpression* expr) override {
                _sequenceVisitor.reset();
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                
                _sequenceVisitor.reset();
                (*expr)[1]->visit(_sequenceVisitor);
                auto right = std::move(_sequenceVisitor.colorTypeSequence);
                
                if (left.size() != 1 && right.size() != 1) {
                    throw base_error("Cannot do ordered comparison of non basic color types");
                }
                evaluation = left[0] <= right[0];  
            }

            void accept(const Colored::EqualityExpression* expr) override {
                _sequenceVisitor.reset();
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                
                _sequenceVisitor.reset();
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
                _sequenceVisitor.reset();
                (*expr)[0]->visit(_sequenceVisitor);
                auto left = std::move(_sequenceVisitor.colorTypeSequence);
                
                _sequenceVisitor.reset();
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

            void accept(const Colored::DotConstantExpression*) override {unexpectedExpression();}
            void accept(const Colored::VariableExpression*) override {unexpectedExpression();}
            void accept(const Colored::UserOperatorExpression*) override {unexpectedExpression();}
            void accept(const Colored::SuccessorExpression*) override {unexpectedExpression();}
            void accept(const Colored::PredecessorExpression*) override {unexpectedExpression();}
            void accept(const Colored::TupleExpression*) override {unexpectedExpression();}
            void accept(const Colored::AllExpression*) override {unexpectedExpression();}
            void accept(const Colored::NumberOfExpression*) override {unexpectedExpression();}
            void accept(const Colored::AddExpression*) override {unexpectedExpression();}
            void accept(const Colored::SubtractExpression*) override {unexpectedExpression();}
            void accept(const Colored::ScalarProductExpression*) override {unexpectedExpression();}

            bool evaluation;
        private:
            const Colored::ColorTypeMap* const _colorTypeMap;
            const Binding* const _binding;
            ColorSeqEvaluationVisitor _sequenceVisitor;
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            
            void unexpectedExpression() {
                throw base_error("unexpected exception");
            }
        };

        GuardExpression::GuardExpression(
            std::shared_ptr<Colored::ColorTypeMap> colorTypeMap,
            Colored::GuardExpression_ptr guardExpression,
            std::shared_ptr<std::unordered_map<std::string, Variable_t>> variableMap
        )
            : _colorTypeMap(std::move(colorTypeMap)),
            _guardExpression(std::move(guardExpression)),
            _variableMap(std::move(variableMap)) {}
        
        bool GuardExpression::eval(const Binding &binding) {
            BooleanEvaluationVisitor visitor(*_colorTypeMap, binding, *_variableMap);
            _guardExpression->visit(visitor);
            return visitor.evaluation;
        }
    }
}

