#include "PetriEngine/ExplicitColored/GuardCompiler.h"
#include "utils/MathExt.h"
namespace PetriEngine {
    namespace ExplicitColored {
        class CompiledGuardAndExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardAndExpression(std::vector<std::unique_ptr<CompiledGuardExpression>> copmiledGuardExpressions)
                : _expressions(std::move(copmiledGuardExpressions)) {}
            bool eval(const Binding& binding) override {
                for (const auto& expression : _expressions) {
                    if (!expression->eval(binding)) {
                        return false;
                    }
                }
                return true;
            }
        private:
            std::vector<std::unique_ptr<CompiledGuardExpression>> _expressions;
        };

        class CompiledGuardOrExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardOrExpression(std::vector<std::unique_ptr<CompiledGuardExpression>> copmiledGuardExpressions)
                : _expressions(std::move(copmiledGuardExpressions)) {}
            bool eval(const Binding& binding) override {
                for (const auto& expression : _expressions) {
                    if (expression->eval(binding)) {
                        return true;
                    }
                }
                return false;
            }
        private:
            std::vector<std::unique_ptr<CompiledGuardExpression>> _expressions;
        };

        typedef uint8_t TypeFlag_t;
        struct TypeFlag {
            static const uint8_t LHS_VAR = 1 << 0;
            static const uint8_t RHS_VAR = 1 << 1;
        };

        union VarOrColor {
            Variable_t variable;
            Color_t color;
        };

        struct VarOrColorWithOffset {
            VarOrColor value;
            ColorOffset_t offset;
            Color_t color_max;

            [[nodiscard]] Color_t getLhs(const Binding& binding, const TypeFlag_t typeFlag) const {
                return (typeFlag & TypeFlag::LHS_VAR)
                    ? signed_wrap(binding.getValue(value.variable) + offset, color_max)
                    : value.color;
            }

            [[nodiscard]] Color_t getRhs(const Binding& binding, const TypeFlag_t typeFlag) const {
                return (typeFlag & TypeFlag::RHS_VAR)
                    ? signed_wrap(binding.getValue(value.variable) + offset, color_max)
                    : value.color;
            }
        };

        class CompiledGuardLessThanExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardLessThanExpression(VarOrColorWithOffset lhs, VarOrColorWithOffset rhs, TypeFlag_t typeFlag)
                : _lhs(std::move(lhs)), _rhs(std::move(rhs)), _typeFlag(typeFlag) { }

            bool eval(const Binding& binding) override {
                return _lhs.getLhs(binding, _typeFlag) < _rhs.getRhs(binding, _typeFlag);
            }
        private:
            VarOrColorWithOffset _lhs;
            VarOrColorWithOffset _rhs;
            TypeFlag_t _typeFlag;
        };

        class CompiledGuardLessThanEqExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardLessThanEqExpression(VarOrColorWithOffset lhs, VarOrColorWithOffset rhs, TypeFlag_t typeFlag)
                            : _lhs(std::move(lhs)), _rhs(std::move(rhs)), _typeFlag(typeFlag) {}

            bool eval(const Binding& binding) override {
                return _lhs.getLhs(binding, _typeFlag) <= _rhs.getRhs(binding, _typeFlag);
            }
        private:
            VarOrColorWithOffset _lhs;
            VarOrColorWithOffset _rhs;
            TypeFlag_t _typeFlag;
        };

        class CompiledGuardEqualityExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardEqualityExpression(std::vector<TypeFlag_t> typeFlags, std::vector<VarOrColorWithOffset> lhs, std::vector<VarOrColorWithOffset> rhs)
                : _typeFlags(std::move(typeFlags)), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

            bool eval(const Binding& binding) override {
                auto typeFlagIt = _typeFlags.begin();
                auto lhsIt = _lhs.begin();
                auto rhsIt = _rhs.begin();
                while (typeFlagIt != _typeFlags.end()) {
                    if (lhsIt->getLhs(binding, *typeFlagIt) != rhsIt->getRhs(binding, *typeFlagIt)) {
                        return false;
                    }
                    ++lhsIt;
                    ++rhsIt;
                    ++typeFlagIt;
                }
                return true;
            }
        private:
            std::vector<TypeFlag_t> _typeFlags;
            std::vector<VarOrColorWithOffset> _lhs;
            std::vector<VarOrColorWithOffset> _rhs;
        };

        class CompiledGuardInequalityExpression : public CompiledGuardExpression {
        public:
            explicit CompiledGuardInequalityExpression(std::vector<TypeFlag_t> typeFlags, std::vector<VarOrColorWithOffset> lhs, std::vector<VarOrColorWithOffset> rhs)
                : _typeFlags(std::move(typeFlags)), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

            bool eval(const Binding& binding) override {
                auto typeFlagIt = _typeFlags.begin();
                auto lhsIt = _lhs.begin();
                auto rhsIt = _rhs.begin();
                while (typeFlagIt != _typeFlags.end()) {
                    if (lhsIt->getLhs(binding, *typeFlagIt) != rhsIt->getRhs(binding, *typeFlagIt)) {
                        return true;
                    }
                    ++lhsIt;
                    ++rhsIt;
                    ++typeFlagIt;
                }
                return false;
            }
        private:
            std::vector<TypeFlag_t> _typeFlags;
            std::vector<VarOrColorWithOffset> _lhs;
            std::vector<VarOrColorWithOffset> _rhs;
        };

        class VarOrColorVisitor : public Colored::ColorExpressionVisitor {
        public:
            VarOrColorVisitor(const Colored::ColorTypeMap& colorTypeMap, const std::unordered_map<std::string, Variable_t>& variable_map)
                : _isVar(false), _result(), _colorTypeMap(colorTypeMap), _variableMap(variable_map) { }

            void accept(const Colored::SuccessorExpression* expr) override {
                expr->child()->visit(*this);
               _result.offset += 1;
            }

            void accept(const Colored::PredecessorExpression* expr) override {
                expr->child()->visit(*this);
                _result.offset -= 1;
            }

            void accept(const Colored::VariableExpression* expr) override {
                const auto test = _variableMap.find(expr->variable()->name);
                if (test == _variableMap.end())
                    throw base_error("Unknown variable");
                _result.value.variable = test->second;
                _result.color_max = expr->getColorType(_colorTypeMap)->size();
                _result.offset = 0;
                _isVar = true;
            }

            void accept(const Colored::UserOperatorExpression* expr) override {
                _result.value.color = expr->user_operator()->getId();
                _result.color_max = expr->getColorType(_colorTypeMap)->size();
                _result.offset = 0;
                _isVar = false;
            }

            void accept(const Colored::DotConstantExpression*) override {unexpectedExpression();}
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
            void accept(const Colored::TupleExpression*) override {unexpectedExpression();};
            VarOrColorWithOffset getResult() {
                if (_isVar)
                    return std::move(_result);
                _result.value.color = signed_wrap((_result.value.color + _result.offset), _result.color_max);
                return std::move(_result);
            }

            bool isVar() {
                return _isVar;
            }
        private:
            bool _isVar;
            VarOrColorWithOffset _result;
            const Colored::ColorTypeMap& _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>& _variableMap;
            static void unexpectedExpression() {
                throw base_error("unexpected exception");
            }
        };


        class TopLevelVisitor : public Colored::ColorExpressionVisitor {
        public:
            TopLevelVisitor(const Colored::ColorTypeMap& colorTypeMap, const std::unordered_map<std::string, Variable_t>& variableMap)
                : _colorTypeMap(colorTypeMap),
                _variableMap(variableMap),
                _innerVisitor(colorTypeMap, variableMap) { }

            void accept(const Colored::LessThanExpression* expr) override {
                auto [lhs, rhs, flag] = parseColorValue(*(*expr)[0], *(*expr)[1]);
                _top = std::make_unique<CompiledGuardLessThanExpression>(std::move(lhs), std::move(rhs), flag);
            }

            void accept(const Colored::LessThanEqExpression* expr) override {
                auto [lhs, rhs, flag] = parseColorValue(*(*expr)[0], *(*expr)[1]);
                _top = std::make_unique<CompiledGuardLessThanEqExpression>(std::move(lhs), std::move(rhs), flag);
            }

            void accept(const Colored::EqualityExpression* expr) override {
                std::vector<VarOrColorWithOffset> lhs;
                std::vector<TypeFlag_t> flags;

                if (const auto& tuple = std::dynamic_pointer_cast<const Colored::TupleExpression>((*expr)[0])) {
                    for (const auto& colorExpr : *tuple) {
                        colorExpr->visit(_innerVisitor);
                        lhs.push_back(_innerVisitor.getResult());
                        flags.push_back(_innerVisitor.isVar() ? TypeFlag::LHS_VAR : 0);
                    }
                } else {
                    (*expr)[0]->visit(_innerVisitor);
                    lhs.push_back(_innerVisitor.getResult());
                    flags.push_back(_innerVisitor.isVar() ? TypeFlag::LHS_VAR : 0);
                }

                std::vector<VarOrColorWithOffset> rhs;

                if (const auto& tuple = std::dynamic_pointer_cast<const Colored::TupleExpression>((*expr)[1])) {
                    if (tuple->size() != rhs.size()) {
                        throw base_error("tuple size mismatch");
                    }
                    size_t i = 0;
                    for (const auto& colorExpr : *tuple) {
                        colorExpr->visit(_innerVisitor);
                        rhs.push_back(_innerVisitor.getResult());
                        flags[i++] |= _innerVisitor.isVar() ? TypeFlag::RHS_VAR : 0;
                    }
                } else {
                    (*expr)[1]->visit(_innerVisitor);
                    rhs.push_back(_innerVisitor.getResult());
                    flags[0] |= _innerVisitor.isVar() ? TypeFlag::RHS_VAR : 0;
                }

                if (rhs.size() != lhs.size() || lhs.size() != flags.size()) {
                    throw base_error("tuple size mismatch");
                }
                _top = std::make_unique<CompiledGuardEqualityExpression>(std::move(flags), std::move(lhs), std::move(rhs));
            }

            void accept(const Colored::InequalityExpression* expr) override {
                std::vector<VarOrColorWithOffset> lhs;
                std::vector<TypeFlag_t> flags;

                if (const auto& tuple = std::dynamic_pointer_cast<const Colored::TupleExpression>((*expr)[0])) {
                    for (const auto& colorExpr : *tuple) {
                        colorExpr->visit(_innerVisitor);
                        lhs.push_back(_innerVisitor.getResult());
                        flags.push_back(_innerVisitor.isVar() ? TypeFlag::LHS_VAR : 0);
                    }
                } else {
                    (*expr)[0]->visit(_innerVisitor);
                    lhs.push_back(_innerVisitor.getResult());
                    flags.push_back(_innerVisitor.isVar() ? TypeFlag::LHS_VAR : 0);
                }

                std::vector<VarOrColorWithOffset> rhs;

                if (const auto& tuple = std::dynamic_pointer_cast<const Colored::TupleExpression>((*expr)[1])) {
                    if (tuple->size() != rhs.size()) {
                        throw base_error("tuple size mismatch");
                    }
                    size_t i = 0;
                    for (const auto& colorExpr : *tuple) {
                        colorExpr->visit(_innerVisitor);
                        rhs.push_back(_innerVisitor.getResult());
                        flags[i++] |= _innerVisitor.isVar() ? TypeFlag::RHS_VAR : 0;
                    }
                } else {
                    (*expr)[1]->visit(_innerVisitor);
                    rhs.push_back(_innerVisitor.getResult());
                    flags[0] |= _innerVisitor.isVar() ? TypeFlag::RHS_VAR : 0;
                }

                if (rhs.size() != lhs.size() || lhs.size() != flags.size()) {
                    throw base_error("tuple size mismatch");
                }
                _top = std::make_unique<CompiledGuardInequalityExpression>(std::move(flags), std::move(lhs), std::move(rhs));
            }

            void accept(const Colored::AndExpression* expr) override {
                std::vector<std::unique_ptr<CompiledGuardExpression>> expressions;
                for (size_t i = 0; i < expr->size(); i++) {
                    _top = nullptr;
                    (*expr)[i]->visit(*this);
                    expressions.emplace_back(std::move(_top));
                }

                _top = std::make_unique<CompiledGuardAndExpression>(std::move(expressions));
            }

            void accept(const Colored::OrExpression* expr) override {
                std::vector<std::unique_ptr<CompiledGuardExpression>> expressions;
                for (size_t i = 0; i < expr->size(); i++) {
                    _top = nullptr;
                    (*expr)[i]->visit(*this);
                    expressions.emplace_back(std::move(_top));
                }
                _top = std::make_unique<CompiledGuardOrExpression>(std::move(expressions));
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

            std::unique_ptr<CompiledGuardExpression> takeCompiled() {
                return std::move(_top);
            }
        private:
            std::tuple<VarOrColorWithOffset, VarOrColorWithOffset, TypeFlag_t>
                parseColorValue(const Colored::Expression& lhs, const Colored::Expression& rhs) {

                lhs.visit(_innerVisitor);
                auto lhsParsed = _innerVisitor.getResult();

                uint8_t flag = _innerVisitor.isVar() ? TypeFlag::LHS_VAR : 0;

                rhs.visit(_innerVisitor);
                auto rhsParsed = _innerVisitor.getResult();

                flag |= _innerVisitor.isVar() ? TypeFlag::RHS_VAR : 0;

                _top = std::make_unique<CompiledGuardLessThanExpression>(std::move(lhsParsed), std::move(rhsParsed), flag);
                return std::make_tuple(std::move(lhsParsed), std::move(rhsParsed), flag);
            }

            const Colored::ColorTypeMap& _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>& _variableMap;
            std::unique_ptr<CompiledGuardExpression> _top;
            VarOrColorVisitor _innerVisitor;
            static void unexpectedExpression() {
                throw base_error("unexpected exception");
            }
        };

        GuardCompiler::GuardCompiler(const std::unordered_map<std::string, Variable_t>& variableMap, const Colored::ColorTypeMap& colorTypeMap)
            : _colorTypeMap(colorTypeMap), _variableMap(variableMap) { }

        std::pair<std::unique_ptr<CompiledGuardExpression>, std::set<Variable_t>> GuardCompiler::compile(const Colored::GuardExpression &colorExpression) {
            VariableExtractorVisitor variableExtractor(_variableMap);
            colorExpression.visit(variableExtractor);
            auto variables = std::move(variableExtractor.collectedVariables);

            TopLevelVisitor topLevelVisitor(_colorTypeMap, _variableMap);
            colorExpression.visit(topLevelVisitor);
            return {topLevelVisitor.takeCompiled(), std::move(variables)};
        }
    }
}