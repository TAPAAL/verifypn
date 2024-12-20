#include "PetriEngine/ExplicitColored/ArcCompiler.h"
#include "PetriEngine/ExplicitColored/VariableExtractorVisitor.h"
namespace PetriEngine {
    namespace ExplicitColored {
        class ParameterizedColorSequenceMultiSet {
        public:
            ParameterizedColorSequenceMultiSet& operator*=(MarkingCount_t factor) {
                for (auto& parameterizedColorSequenceCount : _parameterizedColorSequenceCounts) {
                    parameterizedColorSequenceCount.second *= factor;
                }
                return *this;
            }

            void operator+=(ParameterizedColorSequenceMultiSet& other) {
                for (const auto& otherParameterizedColorSequenceCount : other._parameterizedColorSequenceCounts) {
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
                for (const auto& [otherParameterizedColorSequence, otherCount] : other._parameterizedColorSequenceCounts) {
                    auto local = get(otherParameterizedColorSequence);
                    if (local == nullptr) {
                        _parameterizedColorSequenceCounts.push_back({otherParameterizedColorSequence, -otherCount });
                    } else {
                        local->second -= otherCount;
                    }
                }

                size_t newColorSizesSize = std::max(other._colorSizes.size(), _colorSizes.size());
                _colorSizes.resize(newColorSizesSize);
                for (size_t i = 0; i < _colorSizes.size(); i++) {
                    _colorSizes[i] = std::max(other._colorSizes[i], _colorSizes[i]);
                }
            }

            void add(const std::vector<ParameterizedColor>& parametrizedColorSequence, sMarkingCount_t count) {
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

            std::tuple<std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>>, CPNMultiSet, std::vector<Color_t>> compileArc() {
                CPNMultiSet constant;
                std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>> variableMarkings;
                bool allConstant = true;
                for (auto& [sequence, count] : _parameterizedColorSequenceCounts) {
                    const bool hasVariable = std::any_of(
                        sequence.begin(), sequence.end(),
                        [&](const auto& color) {return color.isVariable;}
                    );
                    if (hasVariable) {
                        allConstant = false;
                        variableMarkings.emplace_back(std::move(sequence), count);
                    } else {
                        std::vector<Color_t> colorSequence;
                        colorSequence.reserve(sequence.size());
                        for (size_t i = 0; i < sequence.size(); ++i) {
                            colorSequence.push_back(signed_wrap(sequence[i].value.color + sequence[i].offset, _colorSizes[i]));
                        }
                        constant.addCount(ColorSequence{std::move(colorSequence)}, count);
                    }
                }
                if (allConstant) {
                    constant.fixNegative();
                }
                return std::make_tuple(std::move(variableMarkings), std::move(constant), std::move(_colorSizes));
            }
        private:
            std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>* get(const std::vector<ParameterizedColor>& key) {
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

            std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>> _parameterizedColorSequenceCounts;
            std::vector<MarkingCount_t> _colorSizes;
        };

        class ColorPreprocessor : public Colored::ColorExpressionVisitor {
        public:
            ColorPreprocessor(
                const std::unordered_map<std::string, Variable_t>& variableMap,
                const Colored::ColorTypeMap& colorTypeMap
            ) : color(ParameterizedColor::fromColor(DOT_COLOR)),
                maxColor(0),
                constant(true),
                _colorTypeMap(&colorTypeMap),
                _variableMap(&variableMap)
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
                constant = false;
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color.isAll()) {
                    return;
                }
                color.offset++;
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                if (color.isAll()) {
                    return;
                }
                color.offset--;
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
            bool constant;
        private:
            const Colored::ColorTypeMap* const _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>* const _variableMap;


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
                        std::cout << "uses all" << std::endl;
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
                for (auto& constituent : *expr) {
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
                result.setColorSize(0, visitor.maxColor);
                result.add(std::vector { visitor.color }, 1);
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                ColorPreprocessor visitor(*_variableMap, _colorTypeMap);
                expr->visit(visitor);
                result.setColorSize(0, visitor.maxColor);
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

        CompiledArc ArcCompiler::compileArc(
            const Colored::ArcExpression_ptr& arcExpression,
            const std::unordered_map<std::string, Variable_t>& variableMap,
            const Colored::ColorTypeMap& colorTypeMap
        ) {
            VariableExtractorVisitor variableExtractor(variableMap);
            arcExpression->visit(variableExtractor);
            std::set<Variable_t> variables = std::move(variableExtractor.collectedVariables);

            ColorTypePreprocessor colorTypePreprocessor(colorTypeMap, variableMap);
            arcExpression->visit(colorTypePreprocessor);
            auto [variableColors, constant, colorSizes] = colorTypePreprocessor.result.compileArc();
            return CompiledArc(
                std::move(variableColors),
                std::move(constant),
                std::move(colorSizes),
                std::move(variables)
            );
        }
    }
}