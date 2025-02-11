#include "PetriEngine/ExplicitColored/ArcCompiler.h"
#include "PetriEngine/ExplicitColored/VariableExtractorVisitor.h"
#include "utils/MathExt.h"
namespace PetriEngine {
    namespace ExplicitColored {
        class ArcExpressionAddition final : public CompiledArcExpression {
        public:
            ArcExpressionAddition(std::unique_ptr<CompiledArcExpression> lhs, std::unique_ptr<CompiledArcExpression> rhs)
                : _lhs(std::move(lhs)), _rhs(std::move(rhs)) {
                _minimalMarkingCount = _lhs->getMinimalMarkingCount() + _rhs->getMinimalMarkingCount();
                _variables = _lhs->getVariables();
                auto rhsSet = _rhs->getVariables();
                _variables.merge(rhsSet);
            }

            const CPNMultiSet& eval(const Binding& binding) const override {
                _result = _lhs->eval(binding);
                _result += _rhs->eval(binding);
                return _result;
            }

            void produce(CPNMultiSet &out, const Binding &binding) const override {
                _lhs->produce(out, binding);
                _rhs->produce(out, binding);
            }

            void consume(CPNMultiSet &out, const Binding &binding) const override {
                _lhs->consume(out, binding);
                _rhs->consume(out, binding);
                out.fixNegative();
            }

            MarkingCount_t getMinimalMarkingCount() const override {
                return _minimalMarkingCount;
            }

            const std::set<Variable_t> & getVariables() const override {
                return _variables;
            }

            std::unique_ptr<CompiledArcExpression>& getLhs() {
                return _lhs;
            }

            std::unique_ptr<CompiledArcExpression>& getRhs() {
                return _rhs;
            }
        private:
            std::unique_ptr<CompiledArcExpression> _lhs;
            std::unique_ptr<CompiledArcExpression> _rhs;
            MarkingCount_t _minimalMarkingCount;
            std::set<Variable_t> _variables;
            mutable CPNMultiSet _result;
        };

        class ArcExpressionSubtraction final : public CompiledArcExpression {
        public:
            ArcExpressionSubtraction(std::unique_ptr<CompiledArcExpression> lhs, std::unique_ptr<CompiledArcExpression> rhs)
                : _lhs(std::move(lhs)), _rhs(std::move(rhs)) {
                auto lhsValue = _lhs->getMinimalMarkingCount();
                auto rhsValue = _rhs->getMinimalMarkingCount();
                if (lhsValue < rhsValue) {
                    _minimalMarkingCount = 0;
                } else {
                    _minimalMarkingCount = lhsValue - rhsValue;
                }
                _variables = _lhs->getVariables();
                auto rhsSet = _rhs->getVariables();
                _variables.merge(rhsSet);
            }

            const CPNMultiSet& eval(const Binding& binding) const override {
                _result = _lhs->eval(binding);
                _rhs->consume(_result, binding);
                return _result;
            }

            void produce(CPNMultiSet &out, const Binding &binding) const override {
                const auto& result = eval(binding);
                const auto beforeCount = out.totalCount();
                out += result;
            }

            void consume(CPNMultiSet &out, const Binding &binding) const override {
                const auto& result = eval(binding);
                out -= result;
                out.fixNegative();
            }

            MarkingCount_t getMinimalMarkingCount() const override {
                return _minimalMarkingCount;
            }

            const std::set<Variable_t> & getVariables() const override {
                return _variables;
            }

            std::unique_ptr<CompiledArcExpression>& getLhs() {
                return _lhs;
            }

            std::unique_ptr<CompiledArcExpression>& getRhs() {
                return _rhs;
            }
        private:
            std::unique_ptr<CompiledArcExpression> _lhs;
            std::unique_ptr<CompiledArcExpression> _rhs;
            MarkingCount_t _minimalMarkingCount;
            mutable CPNMultiSet _result;
            std::set<Variable_t> _variables;
        };

        class ArcExpressionScale final : public CompiledArcExpression {
        public:
            ArcExpressionScale(std::unique_ptr<CompiledArcExpression> expr, const MarkingCount_t n)
                : _expr(std::move(expr)), _scale(n) {
                _minimalMarkingCount = _expr->getMinimalMarkingCount() * n;
                _variables = _expr->getVariables();
            }

            const CPNMultiSet& eval(const Binding& binding) const override {
                _result = _expr->eval(binding);
                _result *= _scale;
                return _result;
            }

            void produce(CPNMultiSet& out, const Binding& binding) const override {
                _result = _expr->eval(binding);
                _result *= _scale;
                out += _result;
            }

            void consume(CPNMultiSet& out, const Binding& binding) const override {
                _result = _expr->eval(binding);
                _result *= _scale;
                out -= _result;
                out.fixNegative();
            }

            MarkingCount_t getMinimalMarkingCount() const override {
                return _minimalMarkingCount;
            }

            const std::set<Variable_t> & getVariables() const override {
                return _variables;
            }

            std::unique_ptr<CompiledArcExpression>& getInner() {
                return _expr;
            }

            MarkingCount_t getScale() {
                return _scale;
            }
        private:
            std::unique_ptr<CompiledArcExpression> _expr;
            MarkingCount_t _scale;
            MarkingCount_t _minimalMarkingCount;
            mutable CPNMultiSet _result;
            std::set<Variable_t> _variables;
        };

        class ArcExpressionConstant final : public CompiledArcExpression {
        public:
            explicit ArcExpressionConstant(CPNMultiSet constant)
                : _constant(std::move(constant)) {
                _minimalMarkingCount = _constant.totalCount();
            }

            const CPNMultiSet& eval(const Binding& binding) const override {
                return _constant;
            }

            void produce(CPNMultiSet& out, const Binding& binding) const override {
                out += _constant;
            }

            void consume(CPNMultiSet& out, const Binding& binding) const override {
                out -= _constant;
                out.fixNegative();
            }

            MarkingCount_t getMinimalMarkingCount() const override {
                return _minimalMarkingCount;
            }

            bool isSubSet(const  CPNMultiSet& superSet, const Binding &binding) const override {
                return _constant <= superSet;
            }

            const std::set<Variable_t> & getVariables() const override {
                return _variables;
            }

            const CPNMultiSet getConstant() const {
                return _constant;
            }
        private:
            CPNMultiSet _constant;
            MarkingCount_t _minimalMarkingCount;
            std::set<Variable_t> _variables;
        };

        class ArcExpressionVariableCollection final : public CompiledArcExpression {
        public:
            ArcExpressionVariableCollection(std::vector<std::vector<ParameterizedColor>> parameterizedColorSequence, std::vector<Color_t> colorSizes, const MarkingCount_t count)
                : _colorSizes(std::move(colorSizes)), _parameterizedColorSequence(std::move(parameterizedColorSequence)), _count(count) {
                _minimalMarkingCount = _parameterizedColorSequence.size() * _count;
                for (const auto& colorSequence : _parameterizedColorSequence) {
                    for (const auto& color : colorSequence) {
                        if (color.isVariable) {
                            _variables.emplace(color.value.variable);
                        }
                    }
                }
            }

            const CPNMultiSet& eval(const Binding& binding) const override {
                _result = {};
                produce(_result, binding);
                return _result;
            }

            void produce(CPNMultiSet &out, const Binding &binding) const override {
                for (const auto& colorSequence : _parameterizedColorSequence) {
                    out.addCount(getColorSequence(colorSequence, binding), _count);
                }
            }

            void consume(CPNMultiSet &out, const Binding &binding) const override {
                for (const auto& colorSequence : _parameterizedColorSequence) {
                    out.addCount(getColorSequence(colorSequence, binding), -_count);
                }
                out.fixNegative();
            }

            MarkingCount_t getMinimalMarkingCount() const override {
                return _minimalMarkingCount;
            }

            const std::set<Variable_t> & getVariables() const override {
                return _variables;
            }

            const std::vector<std::vector<ParameterizedColor>>& getParameterizedColorSequences() const {
                return _parameterizedColorSequence;
            }

            const std::vector<Color_t>& getColorMaxes() const {
                return _colorSizes;
            }

            const MarkingCount_t getCount() const {
                return _count;
            }

        private:
            ColorSequence getColorSequence(const std::vector<ParameterizedColor>& sequence, const Binding& binding) const {
                std::vector<Color_t> colorSequence;
                for (int i = 0; i < sequence.size(); i++) {
                    const auto& parameterizedColor = sequence[i];
                    if (sequence[i].isVariable) {
                        colorSequence.push_back(
                             signed_wrap(binding.getValue(parameterizedColor.value.variable) + parameterizedColor.offset, _colorSizes[i])
                        );
                    } else {
                        colorSequence.push_back(
                             signed_wrap(parameterizedColor.value.color + parameterizedColor.offset, _colorSizes[i])
                        );
                    }
                }
                return ColorSequence { std::move(colorSequence) };
            }

            std::vector<Color_t> _colorSizes;
            std::vector<std::vector<ParameterizedColor>> _parameterizedColorSequence;
            MarkingCount_t _count;
            MarkingCount_t _minimalMarkingCount;
            std::set<Variable_t> _variables;
            mutable CPNMultiSet _result;
        };

        class ArcExpressionColorVisitor final : public Colored::ColorExpressionVisitor {
        public:
            ArcExpressionColorVisitor(
                const Colored::ColorTypeMap& colorTypeMap,
                const std::unordered_map<std::string, Variable_t>& variableMap
            ) : _colorTypeMap(colorTypeMap), _variableMap(variableMap), _parameterizedColor(), _maxColor(0) {}

            void accept(const Colored::DotConstantExpression* expr) override {
                _parameterizedColor = ParameterizedColor::fromColor(DOT_COLOR);
                _maxColor = 1;
            }

            void accept(const Colored::VariableExpression* expr) override {
                const auto varIt = _variableMap.find(expr->variable()->name);
                if (varIt == _variableMap.end())
                    throw base_error("Unknown variable");
                _parameterizedColor = ParameterizedColor::fromVariable(varIt->second);
                _maxColor = expr->getColorType(_colorTypeMap)->size();
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
                if (_parameterizedColor.isAll()) {
                    return;
                }
                _parameterizedColor.offset++;
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                if (_parameterizedColor.isAll()) {
                    return;
                }
                _parameterizedColor.offset--;
            }

            void accept(const Colored::AllExpression* expr) override {
                _parameterizedColor = ParameterizedColor::fromAll();
                _maxColor = expr->size();
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                _parameterizedColor = ParameterizedColor::fromColor(expr->user_operator()->getId());
                _maxColor = expr->getColorType(_colorTypeMap)->size();
            }

            void accept(const Colored::TupleExpression *) override {unexpectedExpression();}
            void accept(const Colored::LessThanExpression *) override {unexpectedExpression();}
            void accept(const Colored::LessThanEqExpression *) override {unexpectedExpression();}
            void accept(const Colored::EqualityExpression *) override {unexpectedExpression();}
            void accept(const Colored::InequalityExpression *) override {unexpectedExpression();}
            void accept(const Colored::AndExpression *) override {unexpectedExpression();}
            void accept(const Colored::OrExpression *) override {unexpectedExpression();}
            void accept(const Colored::NumberOfExpression *) override {unexpectedExpression();}
            void accept(const Colored::AddExpression *) override {unexpectedExpression();}
            void accept(const Colored::SubtractExpression *) override {unexpectedExpression();}
            void accept(const Colored::ScalarProductExpression *) override {unexpectedExpression();}

            const ParameterizedColor& getColor() const {
                return _parameterizedColor;
            }

            Color_t getMaxColor() const {
                return _maxColor;
            }
        private:
            const Colored::ColorTypeMap& _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>& _variableMap;
            ParameterizedColor _parameterizedColor;
            Color_t _maxColor;
            void unexpectedExpression() {
                throw base_error("Unexpected expression");
            }
        };

        class ArcExpressionCompilerVisitor final : public Colored::ColorExpressionVisitor {
        public:
            ArcExpressionCompilerVisitor(
                const Colored::ColorTypeMap& colorTypeMap,
                const std::unordered_map<std::string, Variable_t>& variableMap
            ) : _colorTypeMap(colorTypeMap),
                _variableMap(variableMap),
                _top(nullptr),
                _scale(1)
            {}

            void accept(const Colored::TupleExpression* expr) override {
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                std::vector<Color_t> colorMaxes;
                colorSequences.emplace_back();
                size_t index = 0;
                for (const auto& colorExpr : *expr) {
                    ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                    colorExpr->visit(visitor);
                    colorMaxes.push_back(visitor.getMaxColor());
                    if (visitor.getColor().isAll()) {
                        for (auto& colorSequence : colorSequences) {
                            colorSequence.push_back(ParameterizedColor::fromColor(0));
                        }
                        size_t originalColorSequencesBound = colorSequences.size();
                        for (Color_t c = 1; c <= visitor.getMaxColor(); c++) {
                            for (size_t i = 0; i < originalColorSequencesBound; i++) {
                                auto copy = colorSequences[i];
                                copy.push_back(ParameterizedColor::fromColor(c));
                                colorSequences.push_back(copy);
                            }
                        }
                    } else {
                        for (auto& colorSequence : colorSequences) {
                            colorSequence.push_back(visitor.getColor());
                        }
                    }
                    index++;
                }
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), colorMaxes, _scale);
            }

            void accept(const Colored::NumberOfExpression* expr) override {
                _scale *= expr->number();
                if (expr->size() > 1) {
                    throw base_error("Unsupported net");
                }
                (*expr)[0]->visit(*this);
            }

            void accept(const Colored::AddExpression* expr) override {
                std::vector<std::unique_ptr<CompiledArcExpression>> constituents;
                for (auto innerExpr : (*expr)) {
                    innerExpr->visit(*this);
                    constituents.push_back(std::move(_top));
                }
                if (constituents.size() > 0) {
                    _top = std::move(constituents[0]);
                }
                for (size_t i = 1; i < constituents.size(); i++) {
                    _top = std::make_unique<ArcExpressionAddition>(
                        std::move(_top),
                        std::move(constituents[i])
                    );
                }
            }

            void accept(const Colored::SubtractExpression* expr) override {
                (*expr)[0]->visit(*this);
                auto lhs = std::move(_top);
                (*expr)[1]->visit(*this);
                _top = std::make_unique<ArcExpressionSubtraction>(std::move(lhs), std::move(_top));
            }

            void accept(const Colored::ScalarProductExpression* expr) override {
                _scale *= expr->scalar();
                expr->child()->visit(*this);
            }

            void accept(const Colored::DotConstantExpression* expr) override {
                ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                expr->visit(visitor);
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back({visitor.getColor()});
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), std::vector { visitor.getMaxColor() }, _scale);
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                expr->visit(visitor);
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back({visitor.getColor()});
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), std::vector { visitor.getMaxColor() }, _scale);
            }

            void accept(const Colored::VariableExpression* expr) override {
                ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                expr->visit(visitor);
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back({visitor.getColor()});
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), std::vector { visitor.getMaxColor() }, _scale);
            }

            void accept(const Colored::SuccessorExpression* expr) override {
                ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                expr->visit(visitor);
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back({visitor.getColor()});
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), std::vector { visitor.getMaxColor() }, _scale);
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                ArcExpressionColorVisitor visitor(_colorTypeMap, _variableMap);
                expr->visit(visitor);
                std::vector<std::vector<ParameterizedColor>> colorSequences;
                colorSequences.push_back({visitor.getColor()});
                _top = std::make_unique<ArcExpressionVariableCollection>(std::move(colorSequences), std::vector { visitor.getMaxColor() }, _scale);
            }

            void accept(const Colored::LessThanExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::LessThanEqExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::EqualityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::InequalityExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AllExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::AndExpression* expr) override {unexpectedExpression();}
            void accept(const Colored::OrExpression* expr) override {unexpectedExpression();}

            std::unique_ptr<CompiledArcExpression> takeArcExpression() {
                return std::move(_top);
            }
        private:
            const Colored::ColorTypeMap& _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>& _variableMap;
            std::unique_ptr<CompiledArcExpression> _top;
            MarkingCount_t _scale;
            void unexpectedExpression() {
                throw base_error("Unexpected expression");
            }
        };

        std::unique_ptr<CompiledArcExpression> ArcCompiler::compile(const Colored::ArcExpression_ptr &arcExpression) const {
            ArcExpressionCompilerVisitor visitor(_colorTypeMap, _variableMap);
            arcExpression->visit(visitor);

            auto top = visitor.takeArcExpression();
            replaceConstants(top);
            preCalculate(top);

            return top;
        }

        std::unique_ptr<CompiledArcExpression> ArcCompiler::testCompile() {
            std::unique_ptr<CompiledArcExpression> top = std::make_unique<ArcExpressionSubtraction>(
                std::make_unique<ArcExpressionAddition>(
                    std::make_unique<ArcExpressionVariableCollection>(
                        std::vector<std::vector<ParameterizedColor>> {
                            std::vector {
                                ParameterizedColor::fromColor(0),
                                ParameterizedColor::fromVariable(1),
                            }
                        },
                        std::vector<Color_t> { 4, 4 },
                        1
                    ),
                    std::make_unique<ArcExpressionAddition>(
                        std::make_unique<ArcExpressionVariableCollection>(
                            std::vector<std::vector<ParameterizedColor>> {
                                std::vector {
                                    ParameterizedColor::fromColor(1),
                                    ParameterizedColor::fromVariable(1),
                                }
                            },
                            std::vector<Color_t> { 4, 4 },
                            1
                        ),
                        std::make_unique<ArcExpressionAddition>(
                            std::make_unique<ArcExpressionVariableCollection>(
                                std::vector<std::vector<ParameterizedColor>> {
                                    std::vector {
                                        ParameterizedColor::fromColor(2),
                                        ParameterizedColor::fromVariable(1),
                                    }
                                },
                                std::vector<Color_t> { 4, 4 },
                            1
                            ),
                            std::make_unique<ArcExpressionVariableCollection>(
                                std::vector<std::vector<ParameterizedColor>> {
                                    std::vector {
                                        ParameterizedColor::fromColor(3),
                                        ParameterizedColor::fromVariable(1),
                                    }
                                },
                                std::vector<Color_t> { 4, 4 },
                            1
                            )
                        )
                    )
                ),
                std::make_unique<ArcExpressionVariableCollection>(
                    std::vector<std::vector<ParameterizedColor>> {
                        std::vector {
                            ParameterizedColor::fromVariable(0),
                            ParameterizedColor::fromVariable(1),
                        }
                    },
                    std::vector<Color_t> { 4, 4 },
                    1
                )
            );
            replaceConstants(top);
            preCalculate(top);
            return top;
        }

        void ArcCompiler::replaceConstants(std::unique_ptr<CompiledArcExpression> &top) {
            std::vector<std::unique_ptr<CompiledArcExpression>*> waiting;
            waiting.push_back(&top);

            for (auto next : waiting) {
                if (auto expr = dynamic_cast<ArcExpressionAddition*>(next->get())) {
                    waiting.push_back(&expr->getLhs());
                    waiting.push_back(&expr->getRhs());
                } else if (auto expr = dynamic_cast<ArcExpressionSubtraction*>(next->get())) {
                    waiting.push_back(&expr->getLhs());
                    waiting.push_back(&expr->getRhs());
                } else if (auto expr = dynamic_cast<ArcExpressionScale*>(next->get())) {
                    waiting.push_back(&expr->getInner());
                } else if (auto expr = dynamic_cast<ArcExpressionVariableCollection*>(next->get())) {
                    std::vector<std::vector<ParameterizedColor>> newColorSequences;
                    CPNMultiSet constantMultiSet;
                    for (auto i = 0; i < expr->getParameterizedColorSequences().size(); i++) {
                        const auto& colorSequence = expr->getParameterizedColorSequences()[i];

                        bool hasVariable = false;
                        std::vector<Color_t> newColorSequence;
                        for (const auto& color : colorSequence) {
                            if (color.isVariable) {
                                hasVariable = true;
                                break;
                            }
                            newColorSequence.push_back(signed_wrap(color.value.color + color.offset, expr->getColorMaxes()[i]));
                        }
                        if (!hasVariable) {
                            constantMultiSet.addCount(ColorSequence { newColorSequence }, expr->getCount());
                        } else {
                            newColorSequences.push_back(colorSequence);
                        }
                    }
                    if (constantMultiSet.totalCount() > 0 && newColorSequences.size() > 0) {
                        *next = std::make_unique<ArcExpressionAddition>(
                            std::make_unique<ArcExpressionConstant>(std::move(constantMultiSet)),
                            std::make_unique<ArcExpressionVariableCollection>(
                                std::move(newColorSequences),
                                expr->getColorMaxes(),
                                expr->getCount()
                            )
                        );
                    } else if (constantMultiSet.totalCount() > 0) {
                        *next = std::make_unique<ArcExpressionConstant>(std::move(constantMultiSet));
                    } else if (!newColorSequences.empty()) {
                        *next = std::make_unique<ArcExpressionVariableCollection>(
                            std::move(newColorSequences),
                            expr->getColorMaxes(),
                            expr->getCount()
                        );
                    } else {
                        throw base_error("arc optimization failed");
                    }
                }
            }
        }

        void ArcCompiler::preCalculate(std::unique_ptr<CompiledArcExpression> &top) {
            std::vector<std::unique_ptr<CompiledArcExpression>*> waiting;
            waiting.push_back(&top);
            bool appliedOptimization = true;
            while (appliedOptimization) {
                appliedOptimization = false;
                for (auto next : waiting) {
                    if (auto expr = dynamic_cast<ArcExpressionAddition*>(next->get())) {
                        auto lhs = dynamic_cast<ArcExpressionConstant*>(expr->getLhs().get());
                        auto rhs = dynamic_cast<ArcExpressionConstant*>(expr->getRhs().get());

                        if (lhs && rhs) {
                            auto newMultiSet = lhs->getConstant();
                            newMultiSet += rhs->getConstant();
                            *next = std::make_unique<ArcExpressionConstant>(newMultiSet);
                            appliedOptimization = true;
                        } else {
                            waiting.push_back(&expr->getLhs());
                            waiting.push_back(&expr->getRhs());
                        }
                    } else if (auto expr = dynamic_cast<ArcExpressionSubtraction*>(next->get())) {
                        auto lhs = dynamic_cast<ArcExpressionConstant*>(expr->getLhs().get());
                        auto rhs = dynamic_cast<ArcExpressionConstant*>(expr->getRhs().get());

                        if (lhs && rhs) {
                            auto newMultiSet = lhs->getConstant();
                            newMultiSet -= rhs->getConstant();
                            newMultiSet.fixNegative();
                            *next = std::make_unique<ArcExpressionConstant>(newMultiSet);
                            appliedOptimization = true;
                        } else {
                            waiting.push_back(&expr->getLhs());
                            waiting.push_back(&expr->getRhs());
                        }
                    } else if (auto expr = dynamic_cast<ArcExpressionScale*>(next->get())) {
                        if (auto inner = dynamic_cast<ArcExpressionConstant*>(next->get())) {
                            auto newInner = inner->getConstant();
                            newInner *= expr->getScale();
                            *next = std::make_unique<ArcExpressionConstant>(std::move(newInner));
                            appliedOptimization = true;
                        } else {
                            waiting.push_back(&expr->getInner());
                        }
                    }
                }
            }
        }
    }
}