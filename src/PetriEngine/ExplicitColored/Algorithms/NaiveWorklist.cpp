#ifndef NAIVEWORKLIST_CPP
#define NAIVEWORKLIST_CPP

#include <limits>
#include <stack>
#include "PetriEngine/ExplicitColored/Algorithms/NaiveWorklist.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredMarkingSet.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/ExplicitColored/Algorithms/ColoredSearchTypes.h"

namespace PetriEngine {
    namespace ExplicitColored {
        class ColoredExpressionEvaluator : public PetriEngine::PQL::Visitor {
        public:
            static PetriEngine::ExplicitColored::MarkingCount_t eval(
                const PetriEngine::PQL::Expr_ptr& expr,
                const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices
            ) {
                ColoredExpressionEvaluator visitor{marking, placeNameIndices};
                visit(visitor, expr.get());
                return visitor._evaluated;
            }
        protected:
            explicit ColoredExpressionEvaluator(
                const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices
                ) : _placeNameIndices(placeNameIndices), _evaluated(0), _marking(marking) {}

            void _accept(const PetriEngine::PQL::LiteralExpr *element) override {
                _evaluated = static_cast<PetriEngine::ExplicitColored::MarkingCount_t>(element->value());
            }

            void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {
                auto placeIndexIt = _placeNameIndices.find(*(element->name()));
                if (placeIndexIt == _placeNameIndices.end()) {
                    throw base_error("Unknown place in query ", *(element->name()));
                }
                _evaluated = _marking.markings[placeIndexIt->second].totalCount();
            }

            void _accept(const PetriEngine::PQL::NotCondition *element) override { notSupported("NotCondition"); }
            void _accept(const PetriEngine::PQL::AndCondition *element) override { notSupported("AndCondition"); }
            void _accept(const PetriEngine::PQL::OrCondition *element) override { notSupported("OrCondition"); }
            void _accept(const PetriEngine::PQL::LessThanCondition *element) override { notSupported("LessThanCondition"); }
            void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override { notSupported("LessThanOrEqualCondition"); }
            void _accept(const PetriEngine::PQL::EqualCondition *element) override { notSupported("EqualCondition"); }
            void _accept(const PetriEngine::PQL::NotEqualCondition *element) override { notSupported("NotEqualCondition"); }
            void _accept(const PetriEngine::PQL::DeadlockCondition *element) override { notSupported("DeadlockCondition"); }
            void _accept(const PetriEngine::PQL::EFCondition *condition) override { notSupported("EFCondition"); }
            void _accept(const PetriEngine::PQL::FireableCondition *element) override { notSupported("FireableCondition"); }
            void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
            void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
            void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
            void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
            void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
            void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
            void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
            void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
            void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported("PathQuant"); }
            void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported("ExistPath"); }
            void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported("AllPaths"); }
            void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
            void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
            void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
            void _accept(const PetriEngine::PQL::AGCondition *condition) override  { notSupported("AGCondition"); }
            void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
            void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
            void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
            void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
            void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
            void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported("ACondition"); }
            void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported("ECondition"); }
            void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported("GCondition"); }
            void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported("FCondition"); }
            void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported("XCondition"); }
            void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
            void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
            void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
            void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
            void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
            void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
            void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
            void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
            void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
            void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
            void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
            void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
            void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
            void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
        private:
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
            PetriEngine::ExplicitColored::MarkingCount_t _evaluated;
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;


            void notSupported() {
                throw base_error("Not supported");
            }

            void notSupported(const std::string& type) {
                throw base_error("Not supported ", type);
            }

            void invalid() {
                throw base_error("Invalid expression");
            }
        };

        class GammaQueryVisitor : public PetriEngine::PQL::Visitor {
        public:
            static ConditionalBool eval(
                const PetriEngine::PQL::Condition_ptr& expr,
                const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                ConditionalBool deadlockValue
            ) {
                GammaQueryVisitor visitor{marking, placeNameIndices, deadlockValue};
                visit(visitor, expr);

                return visitor._dependsOnDeadlock
                    ? ConditionalBool::UNKNOWN
                    : (visitor._answer ? ConditionalBool::TRUE : ConditionalBool::FALSE);
            }
        protected:
            explicit GammaQueryVisitor(
                const PetriEngine::ExplicitColored::ColoredPetriNetMarking& marking,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                ConditionalBool deadlockValue
            )
                : _deadlockValue(deadlockValue), _marking(marking), _placeNameIndices(placeNameIndices) { }

            void _accept(const PetriEngine::PQL::NotCondition *element) override {
                visit(this, element->getCond().get());
                _answer = !_answer;
            }

            void _accept(const PetriEngine::PQL::AndCondition *expr) override {
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    if (!_answer) {
                        return;
                    }
                }
            }

            void _accept(const PetriEngine::PQL::OrCondition *expr) override {
                for (const auto& subExpr : *expr) {
                    visit(this, subExpr.get());
                    if (_answer) {
                        return;
                    }
                }
            }

            void _accept(const PetriEngine::PQL::LessThanCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs < rhs;
            }

            void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs <= rhs;
            }

            void _accept(const PetriEngine::PQL::EqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs == rhs;
            }

            void _accept(const PetriEngine::PQL::NotEqualCondition *element) override {
                const auto lhs = ColoredExpressionEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
                const auto rhs = ColoredExpressionEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
                _answer = lhs != rhs;
            }

            void _accept(const PetriEngine::PQL::DeadlockCondition *element) override {
                if (_deadlockValue == ConditionalBool::TRUE) {
                    _answer = true;
                } else if (_deadlockValue == ConditionalBool::FALSE) {
                    _answer = false;
                } else {
                    _dependsOnDeadlock = true;
                }
            }

            void _accept(const PetriEngine::PQL::FireableCondition *element) override {
                notSupported("Does not support fireable");
            }

            void _accept(const PetriEngine::PQL::EFCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PetriEngine::PQL::AGCondition *condition) override {
                notSupported("Does not supported nested quantifiers");
            }

            void _accept(const PetriEngine::PQL::LiteralExpr *element) override {
                invalid();
            }

            void _accept(const PetriEngine::PQL::IdentifierExpr *element) override {
                invalid();
            }

            void _accept(const PetriEngine::PQL::CompareConjunction *element) override  { notSupported("CompareConjunction"); }
            void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override { notSupported("UnfoldedUpperBoundsCondition"); }
            void _accept(const PetriEngine::PQL::CommutativeExpr *element) override  { notSupported("CommutativeExpr"); }
            void _accept(const PetriEngine::PQL::SimpleQuantifierCondition *element) override  { notSupported("SimpleQuantifierCondition"); }
            void _accept(const PetriEngine::PQL::LogicalCondition *element) override  { notSupported("LogicalCondition"); }
            void _accept(const PetriEngine::PQL::CompareCondition *element) override  { notSupported("CompareCondition"); }
            void _accept(const PetriEngine::PQL::UntilCondition *element) override  { notSupported("UntilCondition"); }
            void _accept(const PetriEngine::PQL::ControlCondition *condition) override  { notSupported("ControlCondition"); }
            void _accept(const PetriEngine::PQL::PathQuant *element) override  { notSupported("PathQuant"); }
            void _accept(const PetriEngine::PQL::ExistPath *element) override  { notSupported("ExistPath"); }
            void _accept(const PetriEngine::PQL::AllPaths *element) override  { notSupported("AllPaths"); }
            void _accept(const PetriEngine::PQL::PathSelectCondition *element) override  { notSupported("PathSelectCondition"); }
            void _accept(const PetriEngine::PQL::PathSelectExpr *element) override  { notSupported("PathSelectExpr"); }
            void _accept(const PetriEngine::PQL::EGCondition *condition) override  { notSupported("EGCondition"); }
            void _accept(const PetriEngine::PQL::AFCondition *condition) override  { notSupported("AFCondition"); }
            void _accept(const PetriEngine::PQL::EXCondition *condition) override  { notSupported("EXCondition"); }
            void _accept(const PetriEngine::PQL::AXCondition *condition) override  { notSupported("AXCondition"); }
            void _accept(const PetriEngine::PQL::EUCondition *condition) override  { notSupported("EUCondition"); }
            void _accept(const PetriEngine::PQL::AUCondition *condition) override  { notSupported("AUCondition"); }
            void _accept(const PetriEngine::PQL::ACondition *condition) override  { notSupported("ACondition"); }
            void _accept(const PetriEngine::PQL::ECondition *condition) override  { notSupported("ECondition"); }
            void _accept(const PetriEngine::PQL::GCondition *condition) override  { notSupported("GCondition"); }
            void _accept(const PetriEngine::PQL::FCondition *condition) override  { notSupported("FCondition"); }
            void _accept(const PetriEngine::PQL::XCondition *condition) override  { notSupported("XCondition"); }
            void _accept(const PetriEngine::PQL::ShallowCondition *element) override  { notSupported("ShallowCondition"); }
            void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override  { notSupported("UnfoldedFireableCondition"); }
            void _accept(const PetriEngine::PQL::UpperBoundsCondition *element) override  { notSupported("UpperBoundsCondition"); }
            void _accept(const PetriEngine::PQL::LivenessCondition *element) override  { notSupported("LivenessCondition"); }
            void _accept(const PetriEngine::PQL::KSafeCondition *element) override  { notSupported("KSafeCondition"); }
            void _accept(const PetriEngine::PQL::QuasiLivenessCondition *element) override  { notSupported("QuasiLivenessCondition"); }
            void _accept(const PetriEngine::PQL::StableMarkingCondition *element) override  { notSupported("StableMarkingCondition"); }
            void _accept(const PetriEngine::PQL::BooleanCondition *element) override  { notSupported("BooleanCondition"); }
            void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override  { notSupported("UnfoldedIdentifierExpr"); }
            void _accept(const PetriEngine::PQL::PlusExpr *element) override  { notSupported("PlusExpr"); }
            void _accept(const PetriEngine::PQL::MultiplyExpr *element) override { notSupported("MultiplyExpr"); }
            void _accept(const PetriEngine::PQL::MinusExpr *element) override  { notSupported("MinusExpr"); }
            void _accept(const PetriEngine::PQL::NaryExpr *element) override  { notSupported("NaryExpr"); }
            void _accept(const PetriEngine::PQL::SubtractExpr *element) override { notSupported("SubtractExpr"); }
        private:
            void notSupported() {
                throw base_error("Not supported");
            }
            void notSupported(const std::string& type) {
                throw base_error("Not supported ", type);
            }

            void invalid() {
                throw base_error("Invalid expression");
            }

            bool _answer = true;
            ConditionalBool _deadlockValue;
            bool _dependsOnDeadlock = false;
            const PetriEngine::ExplicitColored::ColoredPetriNetMarking& _marking;
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
        };

        NaiveWorklist::NaiveWorklist(
            const ColoredPetriNet& net,
            const PQL::Condition_ptr &query,
            std::unordered_map<std::string, uint32_t> placeNameIndices
        ) : _net(std::move(net)),
            _placeNameIndices(std::move(placeNameIndices))
        {
            if (const auto efGammaQuery = dynamic_cast<PetriEngine::PQL::EFCondition*>(query.get())) {
                _quantifier = Quantifier::EF;
                _gammaQuery = efGammaQuery->getCond();
            } else if (const auto agGammaQuery = dynamic_cast<PetriEngine::PQL::AGCondition*>(query.get())) {
                _quantifier = Quantifier::AG;
                _gammaQuery = agGammaQuery->getCond();
            } else {
                throw base_error("Unsupported query quantifier");
            }
        }

        bool NaiveWorklist::check(SearchStrategy searchStrategy, size_t randomSeed) {
            switch (searchStrategy) {
                case SearchStrategy::DFS:
                    return _dfs();
                case SearchStrategy::BFS:
                    return _bfs();
                case SearchStrategy::RDFS:
                    return _rdfs(randomSeed);
                default:
                    throw base_error("Unsupported exploration type");
            }
        }

        const SearchStatistics & NaiveWorklist::GetSearchStatistics() const {
            return _searchStatistics;
        }

        ConditionalBool NaiveWorklist::_check(const PetriEngine::ExplicitColored::ColoredPetriNetMarking& state, ConditionalBool deadlockValue) {
            return GammaQueryVisitor::eval(_gammaQuery, state, _placeNameIndices, deadlockValue);
        }

        template<typename WaitingList>
        bool NaiveWorklist::_genericSearch(WaitingList waiting) {
            auto passed = ColoredMarkingSet {};
            const auto& initialState = _net.initial();
            ColoredSuccessorGenerator successorGenerator(_net);
            size_t check_count = 0;
            waiting.add(ColoredPetriNetState { initialState });

            passed.add(initialState);
            _searchStatistics.passedCount = passed.size();

            const auto earlyTerminationCondition = (_quantifier == Quantifier::EF)
                ? ConditionalBool::TRUE
                : ConditionalBool::FALSE;
            if (_check(initialState, ConditionalBool::UNKNOWN) == earlyTerminationCondition){
                return _quantifier == Quantifier::EF;
            }

            while (!waiting.empty()){
                auto& next = waiting.next();

                bool isFirstCheck = next.lastBinding == 0 && next.lastTrans == 0;
                auto successor = successorGenerator.next(next);

                if (isFirstCheck) {
                    const auto deadlockValue = successor.lastTrans == std::numeric_limits<uint32_t>::max();
                    const auto checkValue = _check(successor.marking, deadlockValue ? ConditionalBool::TRUE : ConditionalBool::FALSE);

                    if (checkValue == earlyTerminationCondition) {
                        return _quantifier == Quantifier::EF;
                    }
                }

                if (successor.lastTrans ==  std::numeric_limits<uint32_t>::max()) {
                    waiting.remove();
                    continue;
                }

                auto& marking = successor.marking;

                _searchStatistics.exploredStates++;

                if (!passed.contains(marking)) {
                    if (_check(marking, ConditionalBool::UNKNOWN) == earlyTerminationCondition) {
                        return _quantifier == Quantifier::EF;
                    }
                    _searchStatistics.checkedStates += 1;
                    check_count += 1;
                    passed.add(marking);
                    _searchStatistics.passedCount = passed.size();
                    waiting.add(std::move(successor));
                    _searchStatistics.endWaitingStates = waiting.size();
                    _searchStatistics.peakWaitingStates = std::max(waiting.size(), _searchStatistics.peakWaitingStates);
                }
            }
            return _quantifier == Quantifier::AG;
        }

        bool NaiveWorklist::_dfs() {
            return _genericSearch<DFSStructure>(DFSStructure {});
        }

        bool NaiveWorklist::_bfs() {
            return _genericSearch<BFSStructure>(BFSStructure {});
        }

        bool NaiveWorklist::_rdfs(size_t seed) {
            return _genericSearch<RDFSStructure>(RDFSStructure(seed));
        }
    }
}
#endif //NAIVEWORKLIST_CPP