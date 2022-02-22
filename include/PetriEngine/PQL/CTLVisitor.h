#ifndef VERIFYPN_CTLVISITOR_H
#define VERIFYPN_CTLVISITOR_H

#include "Visitor.h"

namespace PetriEngine::PQL {

    enum CTLSyntaxType {BOOLEAN, PATH, ERROR = -1};

    class IsCTLVisitor : public Visitor {
    public:
        bool isCTL = true;

    protected:
        void _accept(const NotCondition *element) override;

        void _accept(const AndCondition *element) override;

        void _accept(const OrCondition *element) override;

        void _accept(const LessThanCondition *element) override;

        void _accept(const LessThanOrEqualCondition *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const ControlCondition *condition) override;

        void _accept(const ACondition *condition) override;

        void _accept(const ECondition *condition) override;

        void _accept(const GCondition *condition) override;

        void _accept(const FCondition *condition) override;

        void _accept(const XCondition *condition) override;

        void _accept(const UntilCondition *condition) override;

        void _accept(const UnfoldedFireableCondition *element) override;

        void _accept(const FireableCondition *element) override;

        void _accept(const UpperBoundsCondition *element) override;

        void _accept(const LivenessCondition *element) override;

        void _accept(const KSafeCondition *element) override;

        void _accept(const QuasiLivenessCondition *element) override;

        void _accept(const StableMarkingCondition *element) override;

        void _accept(const BooleanCondition *element) override;

        void _accept(const UnfoldedIdentifierExpr *element) override;

        void _accept(const LiteralExpr *element) override;

        void _accept(const PlusExpr *element) override;

        void _accept(const MultiplyExpr *element) override;

        void _accept(const MinusExpr *element) override;

        void _accept(const SubtractExpr *element) override;

        void _accept(const IdentifierExpr *element) override;

    private:
        CTLSyntaxType _cur_type;

        void _accept(const LogicalCondition *element);

        void _accept(const CompareCondition *element);

        void _accept(const ReleaseCondition *condition);
    };
}

#endif //VERIFYPN_CTLVISITOR_H
