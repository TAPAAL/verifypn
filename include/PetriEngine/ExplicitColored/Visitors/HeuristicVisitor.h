#ifndef HEURISTIC_H
#define HEURISTIC_H
#include <PetriEngine/ExplicitColored/AtomicTypes.h>
#include <PetriEngine/ExplicitColored/ColoredPetriNetMarking.h>
#include <PetriEngine/PQL/Visitor.h>

namespace PetriEngine::ExplicitColored {
    class DeltaEvaluator final : public PQL::Visitor {
    public:
        static MarkingCount_t eval(
            const PQL::Expr_ptr& expr,
            const ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
        ) {
            DeltaEvaluator visitor{marking, placeNameIndices};
            visit(visitor, expr.get());
            return visitor._count;
        }

    protected:
        explicit DeltaEvaluator(
            const ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices
        ) : _marking(marking), _placeNameIndices(placeNameIndices) {
        }

        void _accept(const PQL::LiteralExpr* element) override {
            _count = static_cast<MarkingCount_t>(element->value());
        }

        void _accept(const PQL::IdentifierExpr* element) override {
            const auto placeIndexIt = _placeNameIndices.find(*(element->name()));
            if (placeIndexIt == _placeNameIndices.end()) {
                throw base_error("Unknown place in query ", *(element->name()));
            }
            _count = _marking.markings[placeIndexIt->second].totalCount();
        }

        void _accept(const PQL::NotCondition* element) override {
            notSupported("NotCondition");
        }

        void _accept(const PQL::AndCondition* element) override {
            notSupported("AndCondition");
        }

        void _accept(const PQL::OrCondition* element) override {
            notSupported("OrCondition");
        }

        void _accept(const PQL::LessThanCondition* element) override {
            notSupported("LessThanCondition");
        }

        void _accept(const PQL::LessThanOrEqualCondition* element) override {
            notSupported("LessThanOrEqualCondition");
        }

        void _accept(const PQL::EqualCondition* element) override {
            notSupported("EqualCondition");
        }

        void _accept(const PQL::NotEqualCondition* element) override {
            notSupported("NotEqualCondition");
        }

        void _accept(const PQL::DeadlockCondition* element) override {
            notSupported("DeadlockCondition");
        }

        void _accept(const PQL::EFCondition* condition) override {
            notSupported("EFCondition");
        }

        void _accept(const PQL::FireableCondition* element) override {
            notSupported("FireableCondition");
        }

        void _accept(const PQL::CompareConjunction* element) override {
            notSupported("CompareConjunction");
        }

        void _accept(const PQL::UnfoldedUpperBoundsCondition* element) override {
            notSupported("UnfoldedUpperBoundsCondition");
        }

        void _accept(const PQL::CommutativeExpr* element) override {
            notSupported("CommutativeExpr");
        }

        void _accept(const PQL::SimpleQuantifierCondition* element) override {
            notSupported("SimpleQuantifierCondition");
        }

        void _accept(const PQL::LogicalCondition* element) override {
            notSupported("LogicalCondition");
        }

        void _accept(const PQL::CompareCondition* element) override {
            notSupported("CompareCondition");
        }

        void _accept(const PQL::UntilCondition* element) override {
            notSupported("UntilCondition");
        }

        void _accept(const PQL::ControlCondition* condition) override {
            notSupported("ControlCondition");
        }

        void _accept(const PQL::PathQuant* element) override {
            notSupported("PathQuant");
        }

        void _accept(const PQL::ExistPath* element) override {
            notSupported("ExistPath");
        }

        void _accept(const PQL::AllPaths* element) override {
            notSupported("AllPaths");
        }

        void _accept(const PQL::PathSelectCondition* element) override {
            notSupported("PathSelectCondition");
        }

        void _accept(const PQL::PathSelectExpr* element) override {
            notSupported("PathSelectExpr");
        }

        void _accept(const PQL::EGCondition* condition) override {
            notSupported("EGCondition");
        }

        void _accept(const PQL::AGCondition* condition) override {
            notSupported("AGCondition");
        }

        void _accept(const PQL::AFCondition* condition) override {
            notSupported("AFCondition");
        }

        void _accept(const PQL::EXCondition* condition) override {
            notSupported("EXCondition");
        }

        void _accept(const PQL::AXCondition* condition) override {
            notSupported("AXCondition");
        }

        void _accept(const PQL::EUCondition* condition) override {
            notSupported("EUCondition");
        }

        void _accept(const PQL::AUCondition* condition) override {
            notSupported("AUCondition");
        }

        void _accept(const PQL::ACondition* condition) override {
            notSupported("ACondition");
        }

        void _accept(const PQL::ECondition* condition) override {
            notSupported("ECondition");
        }

        void _accept(const PQL::GCondition* condition) override {
            notSupported("GCondition");
        }

        void _accept(const PQL::FCondition* condition) override {
            notSupported("FCondition");
        }

        void _accept(const PQL::XCondition* condition) override {
            notSupported("XCondition");
        }

        void _accept(const PQL::ShallowCondition* element) override {
            notSupported("ShallowCondition");
        }

        void _accept(const PQL::UnfoldedFireableCondition* element) override {
            notSupported("UnfoldedFireableCondition");
        }

        void _accept(const PQL::UpperBoundsCondition* element) override {
            notSupported("UpperBoundsCondition");
        }

        void _accept(const PQL::LivenessCondition* element) override {
            notSupported("LivenessCondition");
        }

        void _accept(const PQL::KSafeCondition* element) override {
            notSupported("KSafeCondition");
        }

        void _accept(const PQL::QuasiLivenessCondition* element) override {
            notSupported("QuasiLivenessCondition");
        }

        void _accept(const PQL::StableMarkingCondition* element) override {
            notSupported("StableMarkingCondition");
        }

        void _accept(const PQL::BooleanCondition* element) override {
            notSupported("BooleanCondition");
        }

        void _accept(const PQL::UnfoldedIdentifierExpr* element) override {
            notSupported("UnfoldedIdentifierExpr");
        }

        void _accept(const PQL::PlusExpr* element) override {
            notSupported("PlusExpr");
        }

        void _accept(const PQL::MultiplyExpr* element) override {
            notSupported("MultiplyExpr");
        }

        void _accept(const PQL::MinusExpr* element) override {
            notSupported("MinusExpr");
        }

        void _accept(const PQL::NaryExpr* element) override {
            notSupported("NaryExpr");
        }

        void _accept(const PQL::SubtractExpr* element) override {
            notSupported("SubtractExpr");
        }

    private:
        MarkingCount_t _count{};
        const ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;

        static void notSupported() {
            throw base_error("Not supported");
        }

        static void notSupported(const std::string& type) {
            throw base_error("Not supported ", type);
        }

        static void invalid() {
            throw base_error("Invalid expression");
        }
    };

    class DistQueryVisitor : public PQL::Visitor {
    public:
        static MarkingCount_t eval(
            const PQL::Condition_ptr& expr,
            const ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const bool neg
        ) {
            DistQueryVisitor visitor{marking, placeNameIndices, neg};
            visit(visitor, expr);
            return visitor._dist;
        }

    protected:
        explicit DistQueryVisitor(
            const ColoredPetriNetMarking& marking,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const bool neg
        )
            : _neg(neg), _marking(marking), _placeNameIndices(placeNameIndices) {
        }

        void _accept(const PQL::NotCondition* element) override {
            _neg = !_neg;
            visit(this, element->getCond().get());
        }

        void _accept(const PQL::AndCondition* expr) override {
            MarkingCount_t sum = 0;
            for (const auto& subExpr : *expr) {
                visit(this, subExpr.get());
                sum += _dist;
            }
            _dist = sum;
        }

        void _accept(const PQL::OrCondition* expr) override {
            MarkingCount_t min = std::numeric_limits<MarkingCount_t>::max();
            for (const auto& subExpr : *expr) {
                visit(this, subExpr.get());
                if (min > _dist) {
                    min = _dist;
                }
            }
            _dist = min;
        }

        void _accept(const PQL::LessThanCondition* element) override {
            const auto lhs = DeltaEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = DeltaEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            int64_t val;
            if (!_neg) {
                val = lhs - rhs + 1;
            }
            else {
                val = rhs - lhs;
            }
            _dist = static_cast<MarkingCount_t>(std::max<int64_t>(val, 0));
        }

        void _accept(const PQL::LessThanOrEqualCondition* element) override {
            const auto lhs = DeltaEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const auto rhs = DeltaEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            int64_t val;
            if (!_neg) {
                val = lhs - rhs;
            }
            else {
                val = rhs - lhs + 1;
            }
            _dist = static_cast<MarkingCount_t>(std::max<int64_t>(val, 0));
        }

        void _accept(const PQL::EqualCondition* element) override {
            const int64_t lhs = DeltaEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const int64_t rhs = DeltaEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            if (!_neg) {
                _dist = std::abs(lhs - rhs);
            }
            else {
                _dist = lhs == rhs ? 1 : 0;
            }
        }

        void _accept(const PQL::NotEqualCondition* element) override {
            const int64_t lhs = DeltaEvaluator::eval(element->getExpr1(), _marking, _placeNameIndices);
            const int64_t rhs = DeltaEvaluator::eval(element->getExpr2(), _marking, _placeNameIndices);
            if (!_neg) {
                _dist = lhs == rhs ? 1 : 0;
            }
            else {
                _dist = std::abs(lhs - rhs);
            }
        }

        void _accept(const PQL::DeadlockCondition* element) override {
            _dist = 1000000;
        }

        void _accept(const PQL::FireableCondition* element) override {
            _dist = 1000000;
        }

        void _accept(const PQL::EFCondition* condition) override {
            notSupported("Does not supported nested quantifiers");
        }

        void _accept(const PQL::AGCondition* condition) override {
            notSupported("Does not supported nested quantifiers");
        }

        void _accept(const PQL::LiteralExpr* element) override {
            invalid();
        }

        void _accept(const PQL::IdentifierExpr* element) override {
            invalid();
        }

        void _accept(const PQL::CompareConjunction* element) override {
            notSupported("CompareConjunction");
        }

        void _accept(const PQL::UnfoldedUpperBoundsCondition* element) override {
            notSupported("UnfoldedUpperBoundsCondition");
        }

        void _accept(const PQL::CommutativeExpr* element) override {
            notSupported("CommutativeExpr");
        }

        void _accept(const PQL::SimpleQuantifierCondition* element) override {
            notSupported("SimpleQuantifierCondition");
        }

        void _accept(const PQL::LogicalCondition* element) override {
            notSupported("LogicalCondition");
        }

        void _accept(const PQL::CompareCondition* element) override {
            notSupported("CompareCondition");
        }

        void _accept(const PQL::UntilCondition* element) override {
            notSupported("UntilCondition");
        }

        void _accept(const PQL::ControlCondition* condition) override {
            notSupported("ControlCondition");
        }

        void _accept(const PQL::PathQuant* element) override {
            notSupported("PathQuant");
        }

        void _accept(const PQL::ExistPath* element) override {
            notSupported("ExistPath");
        }

        void _accept(const PQL::AllPaths* element) override {
            notSupported("AllPaths");
        }

        void _accept(const PQL::PathSelectCondition* element) override {
            notSupported("PathSelectCondition");
        }

        void _accept(const PQL::PathSelectExpr* element) override {
            notSupported("PathSelectExpr");
        }

        void _accept(const PQL::EGCondition* condition) override {
            notSupported("EGCondition");
        }

        void _accept(const PQL::AFCondition* condition) override {
            notSupported("AFCondition");
        }

        void _accept(const PQL::EXCondition* condition) override {
            notSupported("EXCondition");
        }

        void _accept(const PQL::AXCondition* condition) override {
            notSupported("AXCondition");
        }

        void _accept(const PQL::EUCondition* condition) override {
            notSupported("EUCondition");
        }

        void _accept(const PQL::AUCondition* condition) override {
            notSupported("AUCondition");
        }

        void _accept(const PQL::ACondition* condition) override {
            notSupported("ACondition");
        }

        void _accept(const PQL::ECondition* condition) override {
            notSupported("ECondition");
        }

        void _accept(const PQL::GCondition* condition) override {
            notSupported("GCondition");
        }

        void _accept(const PQL::FCondition* condition) override {
            notSupported("FCondition");
        }

        void _accept(const PQL::XCondition* condition) override {
            notSupported("XCondition");
        }

        void _accept(const PQL::ShallowCondition* element) override {
            notSupported("ShallowCondition");
        }

        void _accept(const PQL::UnfoldedFireableCondition* element) override {
            notSupported("UnfoldedFireableCondition");
        }

        void _accept(const PQL::UpperBoundsCondition* element) override {
            notSupported("UpperBoundsCondition");
        }

        void _accept(const PQL::LivenessCondition* element) override {
            notSupported("LivenessCondition");
        }

        void _accept(const PQL::KSafeCondition* element) override {
            notSupported("KSafeCondition");
        }

        void _accept(const PQL::QuasiLivenessCondition* element) override {
            notSupported("QuasiLivenessCondition");
        }

        void _accept(const PQL::StableMarkingCondition* element) override {
            notSupported("StableMarkingCondition");
        }

        void _accept(const PQL::BooleanCondition* element) override {
            notSupported("BooleanCondition");
        }

        void _accept(const PQL::UnfoldedIdentifierExpr* element) override {
            notSupported("UnfoldedIdentifierExpr");
        }

        void _accept(const PQL::PlusExpr* element) override {
            notSupported("PlusExpr");
        }

        void _accept(const PQL::MultiplyExpr* element) override {
            notSupported("MultiplyExpr");
        }

        void _accept(const PQL::MinusExpr* element) override {
            notSupported("MinusExpr");
        }

        void _accept(const PQL::NaryExpr* element) override {
            notSupported("NaryExpr");
        }

        void _accept(const PQL::SubtractExpr* element) override {
            notSupported("SubtractExpr");
        }

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

        MarkingCount_t _dist = true;
        bool _neg;
        const ColoredPetriNetMarking& _marking;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
    };
}

#endif //HEURISTIC_H
