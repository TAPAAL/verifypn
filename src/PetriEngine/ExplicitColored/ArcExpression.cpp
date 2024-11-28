#include "PetriEngine/ExplicitColored/ArcExpression.h"
#include "PetriEngine/ExplicitColored/VariableExtractorVisitor.h"
#include "PetriEngine/ExplicitColored/CompiledArcExpression.hpp"
namespace PetriEngine {
    namespace ExplicitColored {
        class ColorVisitor : public Colored::ColorExpressionVisitor {
        public:
            ColorVisitor(
                const std::unordered_map<std::string, Variable_t>& variableMap,
                const Binding& binding,
                const Colored::ColorTypeMap& colorTypeMap
            ) : _variableMap(&variableMap),
                _binding(&binding),
                _colorTypeMap(&colorTypeMap)
            {}
        
            void accept(const Colored::DotConstantExpression* expr) override {
                color = DOT_COLOR;
            }

            void accept(const Colored::VariableExpression* expr) override {
                const auto varIt = _variableMap->find(expr->variable()->name);
                if (varIt == _variableMap->end())
                    throw base_error("Unknown variable");
                color = _binding->getValue(varIt->second);
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color == ALL_COLOR) {
                    return;
                }

                color = (color + 1) % expr->getColorType(*_colorTypeMap)->size();
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color == ALL_COLOR) {
                    return;
                }
                if (color > 0) {
                    color -= 1;
                } else {
                    color = expr->getColorType(*_colorTypeMap)->size() - 1;
                }
            }
            
            void accept(const Colored::AllExpression* expr) override {
                color = ALL_COLOR;
            }

            void accept(const Colored::UserOperatorExpression* expr) override { 
                color = expr->user_operator()->getId();
            }
            void accept(const Colored::TupleExpression*) override { unexpectedExpression(); }
            void accept(const Colored::LessThanExpression*) override { unexpectedExpression(); }
            void accept(const Colored::LessThanEqExpression*) override { unexpectedExpression(); }
            void accept(const Colored::EqualityExpression*) override { unexpectedExpression(); }
            void accept(const Colored::InequalityExpression*) override { unexpectedExpression(); }
            void accept(const Colored::AndExpression*) override { unexpectedExpression(); }
            void accept(const Colored::OrExpression*) override { unexpectedExpression(); }
            void accept(const Colored::NumberOfExpression*) override { unexpectedExpression(); }
            void accept(const Colored::AddExpression*) override { unexpectedExpression(); }
            void accept(const Colored::SubtractExpression*) override { unexpectedExpression(); }
            void accept(const Colored::ScalarProductExpression*) override { unexpectedExpression(); }

            Color_t color;
        private:
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            const Binding* _binding;
            const Colored::ColorTypeMap* const _colorTypeMap;

            void unexpectedExpression() {
                throw base_error("Unexpected expression");   
            }
        };

        class ColorTypeVisitor : public Colored::ColorExpressionVisitor {
        public:
            ColorTypeVisitor(
                const std::unordered_map<std::string, Variable_t>& variableMap,
                const Binding& binding,
                const Colored::ColorTypeMap& colorTypeMap
            ) : _variableMap(&variableMap),
                _binding(&binding),
                _colorTypeMap(&colorTypeMap)
            {}

            void accept(const Colored::TupleExpression* expr) override {
                std::vector<std::vector<Color_t>> colorSequences;
                colorSequences.push_back(std::vector<Color_t>());
                
                for (const auto& colorExpr : *expr) {
                    ColorVisitor visitor(*_variableMap, *_binding, *_colorTypeMap);
                    colorExpr->visit(visitor);
                    if (visitor.color == ALL_COLOR) {
                        for (auto& colorSequence : colorSequences) {
                            colorSequence.push_back(0);
                        }
                        Color_t colorMax = expr->getColorType(*_colorTypeMap)->size() - 1;
                        size_t originalColorSequencesBound = colorSequences.size();
                        for (Color_t c = 1; c <= colorMax; c++) {
                            for (size_t i = 0; i < originalColorSequencesBound; i++) {
                                auto copy = colorSequences[i];
                                copy.push_back(c);
                                colorSequences.push_back(copy);
                            }
                        }
                    } else {
                        for (std::vector<Color_t>& colorSequence : colorSequences) {
                            colorSequence.push_back(visitor.color);
                        }
                    }
                }
                for (std::vector<Color_t>& colorSequence : colorSequences) {
                    result.setCount(ColorSequence(std::move(colorSequence)), 1);
                }
            }

            void accept(const Colored::NumberOfExpression* expr) override {
                (*expr)[0]->visit(*this);
                result *= expr->number();
            }
            
            void accept(const Colored::AddExpression* expr) override {
                CPNMultiSet sum;
                for (auto constituent : *expr) {
                    constituent->visit(*this);
                    sum += result;
                    result = CPNMultiSet();
                }
                result = std::move(sum);
            }

            void accept(const Colored::SubtractExpression* expr) override {
                (*expr)[1]->visit(*this);
                auto rhs = std::move(result);
                result = CPNMultiSet();
                (*expr)[0]->visit(*this);
                result -= rhs;
            }

            void accept(const Colored::ScalarProductExpression* expr) override {
                expr->child()->visit(*this);
                result *= expr->scalar();
            }

            void accept(const Colored::DotConstantExpression* expr) override {
                result.setCount(ColorSequence({DOT_COLOR}), 1);
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                result.setCount(ColorSequence({expr->user_operator()->getId()}), 1);
            }

            void accept(const Colored::VariableExpression* expr) override {
                const auto variableIt = _variableMap->find(expr->variable()->name);
                if (variableIt == _variableMap->end()) {
                    throw base_error("Unknown variable ", variableIt->second);
                }
                result.setCount(ColorSequence({_binding->getValue(variableIt->second)}), 1);
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                ColorVisitor visitor(*_variableMap, *_binding, *_colorTypeMap);
                expr->visit(visitor);
                result.setCount(ColorSequence({visitor.color}), 1);
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                ColorVisitor visitor(*_variableMap, *_binding, *_colorTypeMap);
                expr->visit(visitor);
                result.setCount(ColorSequence({visitor.color}), 1);
            }

            void accept(const Colored::LessThanExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::LessThanEqExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::EqualityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::InequalityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AllExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AndExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::OrExpression* expr) override {unexpectedExpression();}

            CPNMultiSet result;
        private:
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            const Binding* const _binding;
            const Colored::ColorTypeMap* const _colorTypeMap;

            void unexpectedExpression() {
                throw base_error("Unexpected expression");   
            }
        };

        ArcExpression::ArcExpression(const Colored::ArcExpression_ptr& arcExpression, const Colored::ColorTypeMap& colorTypeMap, const std::unordered_map<std::string, Variable_t>& variableMap) {
            VariableExtractorVisitor variableExtractor(variableMap);
            arcExpression->visit(variableExtractor);
            _variables = std::move(variableExtractor.collectedVariables);
            ColorTypePreprocessor colorTypePreprocessor(colorTypeMap, variableMap);
            arcExpression->visit(colorTypePreprocessor);
            _parameterizedColorSequenceMultiSet = std::move(colorTypePreprocessor.result);
        }

        CPNMultiSet ArcExpression::eval(const Binding& binding) const {
            return _parameterizedColorSequenceMultiSet.eval(binding);
        }

        const std::set<Variable_t>& ArcExpression::getVariables() const {
            return _variables;
        }
    }
}