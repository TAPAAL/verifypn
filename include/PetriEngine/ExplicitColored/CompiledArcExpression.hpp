#ifndef COMPILED_ARC_EXPRESSION
#define COMPILED_ARC_EXPRESSION
#include "../Colored/Expressions.h"
namespace PetriEngine {
    namespace ExplicitColored {
        union ColorOrVariable {
            Color_t color;
            Variable_t variable;
        };

        struct ParameterizedColor {
            int64_t _offset;
            bool isVariable;
            //perhaps include "all" value so we dont need to materialize all colors
            ColorOrVariable value;
            bool operator==(const ParameterizedColor &other) const {
                return isVariable == other.isVariable
                    && (isVariable
                        ? (value.variable == other.value.variable)
                        : (value.color == other.value.color)
                        );
            }

            bool operator!=(const ParameterizedColor &other) const {
                return !(*this == other);
            }

            bool isAll() const {
                return !isVariable && value.color == ALL_COLOR;
            }

            static ParameterizedColor fromColor(const Color_t color) {
                ParameterizedColor rv;
                rv.isVariable = false;
                rv._offset = 0;
                rv.value.color = color;
                return rv;
            }

            static ParameterizedColor fromVariable(const Variable_t variable) {
                ParameterizedColor rv;
                rv.isVariable = true;
                rv._offset = 0;
                rv.value.variable = variable;
                return rv;
            }

            static ParameterizedColor fromAll() {
                ParameterizedColor rv;
                rv.isVariable = false;
                rv._offset = 0;
                rv.value.color = ALL_COLOR;
                return rv;
            }
        };

        class ParameterizedColorSequenceMultiSet {
        public:
            CPNMultiSet eval(const Binding& binding) const {
                CPNMultiSet result;
                for (auto [parameterizedColorSequence, count] : _parameterizedColorSequenceCounts) {
                    std::vector<Color_t> colorSequence;
                    for (size_t colori = 0; colori < parameterizedColorSequence.size(); colori++) {
                        const ParameterizedColor& color = parameterizedColorSequence[colori];
                        Color_t colorValue;
                        if (color.isVariable) {
                            colorValue = binding.getValue(color.value.variable);
                        } else {
                            colorValue = (color.value.color);
                        }
                        //Keeps the value between 0 - max with correct wrap around handling
                        colorSequence.push_back(((colorValue % _colorSizes[colori]) + _colorSizes[colori]) % _colorSizes[colori]);
                    }
                    result.addCount(ColorSequence(std::move(colorSequence)), count);
                }
                return result;
            }

            ParameterizedColorSequenceMultiSet& operator*=(MarkingCount_t factor) {
                for (auto parameterizedColorSequenceCount : _parameterizedColorSequenceCounts) {
                    parameterizedColorSequenceCount.second *= factor;
                }
                return *this;
            }

            void operator+=(ParameterizedColorSequenceMultiSet& other) {
                for (auto otherParameterizedColorSequenceCount : other._parameterizedColorSequenceCounts) {
                    auto local = get(otherParameterizedColorSequenceCount.first);
                    if (local == nullptr) {
                        _parameterizedColorSequenceCounts.push_back(otherParameterizedColorSequenceCount);
                    } else {
                        local->second += otherParameterizedColorSequenceCount.second;
                    }
                }

                size_t newColorSizesSize = std::max(other._colorSizes.size(), _colorSizes.size());
                _colorSizes.resize(newColorSizesSize);
                for (size_t i = 0; i < _colorSizes.size(); i++) {
                    _colorSizes[i] = std::max(other._colorSizes[i], _colorSizes[i]);
                }
            }

            void operator-=(ParameterizedColorSequenceMultiSet& other) {
                for (auto otherParameterizedColorSequenceCount : other._parameterizedColorSequenceCounts) {
                    auto local = get(otherParameterizedColorSequenceCount.first);
                    if (local == nullptr) {
                        _parameterizedColorSequenceCounts.push_back(otherParameterizedColorSequenceCount);
                    } else {
                        local->second -= otherParameterizedColorSequenceCount.second;
                    }
                }

                size_t newColorSizesSize = std::max(other._colorSizes.size(), _colorSizes.size());
                _colorSizes.resize(newColorSizesSize);
                for (size_t i = 0; i < _colorSizes.size(); i++) {
                    _colorSizes[i] = std::max(other._colorSizes[i], _colorSizes[i]);
                }
            }

            void add(const std::vector<ParameterizedColor>& parametrizedColorSequence, int64_t count) {
                auto local = get(parametrizedColorSequence);
                if (local == nullptr) {
                    _parameterizedColorSequenceCounts.push_back(std::make_pair(parametrizedColorSequence, count));
                } else {
                    local->second += count;
                }
            }

            void setColorSize(size_t colorIndex, MarkingCount_t size) {
                if (_colorSizes.size() <= colorIndex) {
                    _colorSizes.resize(colorIndex + 1);
                    _colorSizes[colorIndex] = size;
                }
            }
        private:
            std::pair<std::vector<ParameterizedColor>, int64_t>* get(const std::vector<ParameterizedColor>& key) {
                for (auto& parameterizedColorSequenceCount : _parameterizedColorSequenceCounts) {
                    if (key.size() != parameterizedColorSequenceCount.first.size()) {
                        continue;
                    }
                    bool match = true;
                    for (size_t i = 0; i < key.size(); i++) {
                        if (key[i] != parameterizedColorSequenceCount.first[i]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        return &parameterizedColorSequenceCount;
                    }
                }
                return nullptr;
            }

            std::vector<std::pair<std::vector<ParameterizedColor>, int64_t>> _parameterizedColorSequenceCounts;
            std::vector<MarkingCount_t> _colorSizes;
        };

        class ColorPreprocessor : public Colored::ColorExpressionVisitor {
        public:
            ColorPreprocessor(
                const std::unordered_map<std::string, Variable_t>& variableMap,
                const Colored::ColorTypeMap& colorTypeMap
            ) : color(ParameterizedColor::fromColor(DOT_COLOR)),
                _variableMap(&variableMap),
                _colorTypeMap(&colorTypeMap),
                maxColor(0)
            {}

            void accept(const Colored::DotConstantExpression* expr) override {
                color = ParameterizedColor::fromColor(DOT_COLOR);
                maxColor = 1;
            }

            void accept(const Colored::VariableExpression* expr) override {
                const auto varIt = _variableMap->find(expr->variable()->name);
                if (varIt == _variableMap->end())
                    throw base_error("Unknown variable");
                color = ParameterizedColor::fromVariable(varIt->second);
                maxColor = expr->getColorType(*_colorTypeMap)->size();
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color.isAll()) {
                    return;
                }
                color._offset++;
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color.isAll()) {
                    return;
                }
                color._offset--;
            }

            void accept(const Colored::AllExpression* expr) override {
                color = ParameterizedColor::fromAll();
                maxColor = expr->size();
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                color = ParameterizedColor::fromColor(expr->user_operator()->getId());
                maxColor = expr->getColorType(*_colorTypeMap)->size();
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

            ParameterizedColor color;
            Color_t maxColor;
        private:
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            const Colored::ColorTypeMap* const _colorTypeMap;

            void unexpectedExpression() {
                throw base_error("Unexpected expression");
            }
        };

        class ColorTypePreprocessor : public Colored::ColorExpressionVisitor {
        public:
            ColorTypePreprocessor(
                const Colored::ColorTypeMap& colorTypeMap,
                const std::unordered_map<std::string, Variable_t>& variableMap
            ) : _variableMap(&variableMap),
                _colorTypeMap(colorTypeMap)
            {}

            void accept(const Colored::TupleExpression* expr) override {
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back(std::vector<ParameterizedColor>());
                size_t index = 0;
                for (const auto& colorExpr : *expr) {
                    ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                    colorExpr->visit(visitor);
                    result.setColorSize(index, visitor.maxColor);
                    if (visitor.color.isAll()) {
                        for (auto& colorSequence : colorSequences) {
                            colorSequence.push_back(ParameterizedColor::fromColor(0));
                        }
                        size_t originalColorSequencesBound = colorSequences.size();
                        for (Color_t c = 1; c <= visitor.maxColor; c++) {
                            for (size_t i = 0; i < originalColorSequencesBound; i++) {
                                auto copy = colorSequences[i];
                                copy.push_back(ParameterizedColor::fromColor(c));
                                colorSequences.push_back(copy);
                            }
                        }
                    } else {
                        for (auto& colorSequence : colorSequences) {
                            colorSequence.push_back(visitor.color);
                        }
                    }
                    index++;
                }
                for (auto& colorSequence : colorSequences) {
                    result.add(colorSequence, 1);
                }
            }

            void accept(const Colored::NumberOfExpression* expr) override {
                (*expr)[0]->visit(*this);
                result *= expr->number();
            }

            void accept(const Colored::AddExpression* expr) override {
                ParameterizedColorSequenceMultiSet sum;
                for (auto constituent : *expr) {
                    constituent->visit(*this);
                    sum += result;
                    result = ParameterizedColorSequenceMultiSet();
                }
                result = std::move(sum);
            }

            void accept(const Colored::SubtractExpression* expr) override {
                (*expr)[1]->visit(*this);
                auto rhs = std::move(result);
                result = ParameterizedColorSequenceMultiSet();
                (*expr)[0]->visit(*this);
                result -= rhs;
            }

            void accept(const Colored::ScalarProductExpression* expr) override {
                expr->child()->visit(*this);
                result *= expr->scalar();
            }

            void accept(const Colored::DotConstantExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.setColorSize(0, 1);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.setColorSize(0, visitor.maxColor);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::VariableExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.setColorSize(0, visitor.maxColor);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::LessThanExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::LessThanEqExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::EqualityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::InequalityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AllExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AndExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::OrExpression* expr) override {unexpectedExpression();}

             ParameterizedColorSequenceMultiSet result;
        private:
            const std::unordered_map<std::string, Variable_t>* const _variableMap;
            const Colored::ColorTypeMap& _colorTypeMap;

            void unexpectedExpression() {
                throw base_error("Unexpected expression");
            }
        };
    }
}

#endif