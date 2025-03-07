#include "PetriEngine/ExplicitColored/GammaQueryCompiler.h"

#include <PetriEngine/ExplicitColored/FireabilityChecker.h>
#include <PetriEngine/PQL/Visitor.h>

#include "PetriEngine/PQL/Expressions.h"
namespace PetriEngine::ExplicitColored {
    MarkingCount_t minShortCircuit(
        const ColoredPetriNetMarking& marking,
        const std::vector<std::unique_ptr<CompiledGammaQueryExpression>>& expressions,
        const bool neg
    ) {
        auto min = std::numeric_limits<MarkingCount_t>::max();
        for (const auto& expression : expressions) {
            const auto dist = expression->distance(marking, neg);
            if (dist == 0) {
                return 0;
            }
            if (dist < min) {
                min = dist;
            }
        }
        return min;
    }

    class GammaQueryAndExpression final : public CompiledGammaQueryExpression {
    public:
        explicit GammaQueryAndExpression(std::vector<std::unique_ptr<CompiledGammaQueryExpression>> expressions)
            : _expressions(std::move(expressions)) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            for (const auto& expression : _expressions) {
                if (!expression->eval(successorGenerator, marking)) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking,
            const bool neg) const override {
            if (neg) {
                return minShortCircuit(marking, _expressions, true);
            }
            return std::accumulate(_expressions.begin(), _expressions.end(), 0,
                [&](const MarkingCount_t sum, const std::unique_ptr<CompiledGammaQueryExpression> &expression) {
                    return sum + expression->distance(marking, false);
                }
            );
        }

    private:
        std::vector<std::unique_ptr<CompiledGammaQueryExpression>> _expressions;
    };

    class GammaQueryOrExpression final : public CompiledGammaQueryExpression {
    public:
        explicit GammaQueryOrExpression(std::vector<std::unique_ptr<CompiledGammaQueryExpression>> expressions)
            : _expressions(std::move(expressions)) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            for (const auto& expression : _expressions) {
                if (expression->eval(successorGenerator, marking)) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking,
            const bool neg) const override {
            if (neg) {
                return std::accumulate(_expressions.begin(), _expressions.end(), 0,
                    [&](const MarkingCount_t sum, const std::unique_ptr<CompiledGammaQueryExpression> &expression) {
                        return sum + expression->distance(marking, true);
                    }
                );
            }
            return minShortCircuit(marking, _expressions, false);
        }

    private:
        std::vector<std::unique_ptr<CompiledGammaQueryExpression>> _expressions;
    };

    class GammaQueryNotExpression final : public CompiledGammaQueryExpression {
    public:
        explicit GammaQueryNotExpression(std::unique_ptr<CompiledGammaQueryExpression> inner)
            : _inner(std::move(inner)){}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return !_inner->eval(successorGenerator, marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking,
            const bool neg) const override {
            return _inner->distance(marking, !neg);
        }

    private:
        std::unique_ptr<CompiledGammaQueryExpression> _inner;
    };

    class QueryValue {
    public:
        static QueryValue fromPlace(const uint32_t placeIndex) {
            QueryValue rv;
            rv._value.placeIndex = placeIndex;
            rv._isPlace = true;
            return rv;
        }

        static QueryValue fromCount(const MarkingCount_t count) {
            QueryValue rv;
            rv._value.count = count;
            rv._isPlace = false;
            return rv;
        }

        static QueryValue fromExpression(const PQL::Expr& expression, const std::unordered_map<std::string, uint32_t>& placeNameIndices) {
            if (const auto literalExpression = dynamic_cast<const PQL::LiteralExpr*>(&expression)) {
                return fromCount(literalExpression->value());
            }
            if (const auto identifierExpression = dynamic_cast<const PQL::IdentifierExpr*>(&expression)) {
                const auto placeIndex = placeNameIndices.find(*identifierExpression->name());
                if (placeIndex == placeNameIndices.end()) {
                    throw base_error("Unknown place '", *identifierExpression->name(), "'");
                }
                return fromPlace(placeIndex->second);
            } else {
                throw base_error("Invalid expression type");
            }
        }

        [[nodiscard]] MarkingCount_t getCount(const ColoredPetriNetMarking& marking) const {
            if (_isPlace) {
                return marking.getPlaceCount(_value.placeIndex);
            } else {
                return _value.count;
            }
        }
    private:
        bool _isPlace = false;
        union {
            MarkingCount_t count;
            uint32_t placeIndex;
        } _value {};
    };

    class GammaQueryLessThanExpression final : public CompiledGammaQueryExpression {
    public:
        GammaQueryLessThanExpression(QueryValue lhs, QueryValue rhs)
            : _lhs(lhs), _rhs(rhs) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return _lhs.getCount(marking) < _rhs.getCount(marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking,
            const bool neg) const override {
            const auto lhs = _lhs.getCount(marking);
            const auto rhs = _rhs.getCount(marking);
            if (neg) {
                if (lhs >= rhs) {
                    return 0;
                }
                return rhs - lhs;
            }
            if (lhs < rhs) {
                return 0;
            }
            return lhs - rhs + 1;
        }

    private:
        QueryValue _lhs;
        QueryValue _rhs;
    };

    class GammaQueryLessThanOrEqualExpression final : public CompiledGammaQueryExpression {
    public:
        GammaQueryLessThanOrEqualExpression(const QueryValue lhs, const QueryValue rhs)
            : _lhs(lhs), _rhs(rhs) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return _lhs.getCount(marking) <= _rhs.getCount(marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking,
            const bool neg) const override {
            const auto lhs = _lhs.getCount(marking);
            const auto rhs = _rhs.getCount(marking);
            if (neg) {
                if (lhs > rhs) {
                    return 0;
                }
                return rhs - lhs + 1;
            }
            if (lhs <= rhs) {
                return 0;
            }
            return lhs - rhs;
        }

    private:
        QueryValue _lhs;
        QueryValue _rhs;
    };

    class GammaQueryEqualExpression final : public CompiledGammaQueryExpression {
    public:
        GammaQueryEqualExpression(const QueryValue lhs, const QueryValue rhs)
            : _lhs(lhs), _rhs(rhs) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return _lhs.getCount(marking) == _rhs.getCount(marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking, const bool neg) const override {
            const auto lhs = _lhs.getCount(marking);
            const auto rhs = _rhs.getCount(marking);
            if (neg)
                return lhs == rhs ? 1 : 0;
            return lhs > rhs ? lhs - rhs : rhs - lhs;
        }

    private:
        QueryValue _lhs;
        QueryValue _rhs;
    };

    class GammaQueryNotEqualExpression final : public CompiledGammaQueryExpression {
    public:
        GammaQueryNotEqualExpression(const QueryValue lhs, const QueryValue rhs)
            : _lhs(lhs), _rhs(rhs) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return _lhs.getCount(marking) != _rhs.getCount(marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking, const bool neg) const override {
            const auto lhs = _lhs.getCount(marking);
            const auto rhs = _rhs.getCount(marking);
            if (neg)
                return lhs > rhs ? lhs - rhs : rhs - lhs;
            return lhs == rhs ? 1 : 0;
        }

    private:
        QueryValue _lhs;
        QueryValue _rhs;
    };

    class GammaQueryDeadlockExpression final : public CompiledGammaQueryExpression {
    public:
        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
           return FireabilityChecker::hasDeadlock(successorGenerator, marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking, const bool neg) const override {
            return 0;
        }
    };

    class GammaQueryFireabilityExpression final : public CompiledGammaQueryExpression {
    public:
        explicit GammaQueryFireabilityExpression(const Transition_t transitionId)
            : _transitionId(transitionId) {}

        [[nodiscard]] bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking &marking) const override {
            return FireabilityChecker::canFire(successorGenerator, _transitionId, marking);
        }

        [[nodiscard]] MarkingCount_t distance(const ColoredPetriNetMarking &marking, bool neg) const override {
            return 0;
        }

    private:
        Transition_t _transitionId;
    };
    
    class GammaQueryCompilerVisitor final : public PQL::Visitor {
        public:
            static std::unique_ptr<CompiledGammaQueryExpression> compile(
                const PQL::Condition_ptr& expr,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
                const ColoredSuccessorGenerator& successorGenerator
            ) {
                GammaQueryCompilerVisitor visitor{placeNameIndices, transitionNameIndices, successorGenerator};
                visit(visitor, expr);

                return std::move(visitor._compiled);
            }
        protected:
            explicit GammaQueryCompilerVisitor(
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
                const ColoredSuccessorGenerator& successorGenerator
            )
                : _successorGenerator(successorGenerator), _placeNameIndices(placeNameIndices), _transitionNameIndices(transitionNameIndices) { }

            void _accept(const PQL::NotCondition *element) override {
                visit(this, element->getCond().get());
                _compiled = std::make_unique<GammaQueryNotExpression>(std::move(_compiled));
            }

            void _accept(const PQL::AndCondition *expr) override {
                std::vector<std::unique_ptr<CompiledGammaQueryExpression>> expressions;
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    expressions.emplace_back(std::move(_compiled));
                }
                _compiled = std::make_unique<GammaQueryAndExpression>(std::move(expressions));
            }

            void _accept(const PQL::OrCondition *expr) override {
                std::vector<std::unique_ptr<CompiledGammaQueryExpression>> expressions;
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    expressions.emplace_back(std::move(_compiled));
                }
                _compiled = std::make_unique<GammaQueryOrExpression>(std::move(expressions));
            }

            void _accept(const PQL::LessThanCondition *element) override {
                _compiled = std::make_unique<GammaQueryLessThanExpression>(
                    QueryValue::fromExpression(*element->getExpr1(), _placeNameIndices),
                    QueryValue::fromExpression(*element->getExpr2(), _placeNameIndices)
                );
            }

            void _accept(const PQL::LessThanOrEqualCondition *element) override {
                _compiled = std::make_unique<GammaQueryLessThanOrEqualExpression>(
                    QueryValue::fromExpression(*element->getExpr1(), _placeNameIndices),
                    QueryValue::fromExpression(*element->getExpr2(), _placeNameIndices)
                );
            }

            void _accept(const PQL::EqualCondition *element) override {
                _compiled = std::make_unique<GammaQueryEqualExpression>(
                    QueryValue::fromExpression(*element->getExpr1(), _placeNameIndices),
                    QueryValue::fromExpression(*element->getExpr2(), _placeNameIndices)
                );
            }

            void _accept(const PQL::NotEqualCondition *element) override {
                _compiled = std::make_unique<GammaQueryNotEqualExpression>(
                    QueryValue::fromExpression(*element->getExpr1(), _placeNameIndices),
                    QueryValue::fromExpression(*element->getExpr2(), _placeNameIndices)
                );
            }

            void _accept(const PQL::DeadlockCondition *element) override {
                _compiled = std::make_unique<GammaQueryDeadlockExpression>();
            }

            void _accept(const PQL::FireableCondition *element) override {
                const auto transitionNameIndex = _transitionNameIndices.find(*element->getName());
                if (transitionNameIndex == _transitionNameIndices.end()) {
                    throw base_error("Unknown transition name '", *element->getName(), "'");
                }
                _compiled = std::make_unique<GammaQueryFireabilityExpression>(transitionNameIndex->second);
            }

            void _accept(const PQL::EFCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PQL::AGCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PQL::LiteralExpr *element) override {
                invalid();
            }

            void _accept(const PQL::IdentifierExpr *element) override {
                invalid();
            }

            void _accept(const PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
            void _accept(const PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
            void _accept(const PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
            void _accept(const PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
            void _accept(const PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
            void _accept(const PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
            void _accept(const PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
            void _accept(const PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
            void _accept(const PQL::PathQuant *element) override  { notSupported("PathQuant"); }
            void _accept(const PQL::ExistPath *element) override  { notSupported("ExistPath"); }
            void _accept(const PQL::AllPaths *element) override  { notSupported("AllPaths"); }
            void _accept(const PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
            void _accept(const PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
            void _accept(const PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
            void _accept(const PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
            void _accept(const PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
            void _accept(const PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
            void _accept(const PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
            void _accept(const PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
            void _accept(const PQL::ACondition *condition) override  { notSupported("ACondition"); }
            void _accept(const PQL::ECondition *condition) override  { notSupported("ECondition"); }
            void _accept(const PQL::GCondition *condition) override  { notSupported("GCondition"); }
            void _accept(const PQL::FCondition *condition) override  { notSupported("FCondition"); }
            void _accept(const PQL::XCondition *condition) override  { notSupported("XCondition"); }
            void _accept(const PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
            void _accept(const PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
            void _accept(const PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
            void _accept(const PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
            void _accept(const PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
            void _accept(const PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
            void _accept(const PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
            void _accept(const PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
            void _accept(const PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
            void _accept(const PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
            void _accept(const PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
            void _accept(const PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
            void _accept(const PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
            void _accept(const PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
        private:
            static void notSupported() {
                throw base_error("Not supported");
            }

            static void notSupported(const std::string& type) {
                throw base_error("Not supported ", type);
            }

            static void invalid() {
                throw base_error("Invalid expression");
            }

            const ColoredSuccessorGenerator& _successorGenerator;
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
            const std::unordered_map<std::string, uint32_t>& _transitionNameIndices;
            std::unique_ptr<CompiledGammaQueryExpression> _compiled;
        };


    GammaQueryCompiler::GammaQueryCompiler(
        const std::unordered_map<std::string, uint32_t>& placeNameIndices,
        const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
        const ColoredSuccessorGenerator& successorGenerator
    ) : _placeNameIndices(placeNameIndices), _transitionNameIndices(transitionNameIndices), _successorGenerator(successorGenerator) {}

    std::unique_ptr<CompiledGammaQueryExpression> GammaQueryCompiler::compile(const PQL::Condition_ptr &expression) const {
        return GammaQueryCompilerVisitor::compile(expression, _placeNameIndices, _transitionNameIndices, _successorGenerator);
    }
}
