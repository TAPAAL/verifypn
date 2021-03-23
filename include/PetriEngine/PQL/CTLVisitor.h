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

        void _accept(const GreaterThanCondition *element) override;

        void _accept(const GreaterThanOrEqualCondition *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const EFCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const AGCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const EUCondition *condition) override;

        void _accept(const AUCondition *condition) override;

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
    };

    class AsCTL : public Visitor {
    public:
        Condition_ptr _ctl_query = nullptr;
        Expr_ptr _expression = nullptr;

    protected:
        void _accept(const NotCondition *element) override;

        void _accept(const AndCondition *element) override;

        void _accept(const OrCondition *element) override;

        void _accept(const LessThanCondition *element) override;

        void _accept(const LessThanOrEqualCondition *element) override;

        void _accept(const GreaterThanCondition *element) override;

        void _accept(const GreaterThanOrEqualCondition *element) override;

        void _accept(const EqualCondition *element) override;

        void _accept(const NotEqualCondition *element) override;

        void _accept(const DeadlockCondition *element) override;

        void _accept(const CompareConjunction *element) override;

        void _accept(const UnfoldedUpperBoundsCondition *element) override;

        void _accept(const EFCondition *condition) override;

        void _accept(const EGCondition *condition) override;

        void _accept(const AGCondition *condition) override;

        void _accept(const AFCondition *condition) override;

        void _accept(const EXCondition *condition) override;

        void _accept(const AXCondition *condition) override;

        void _accept(const EUCondition *condition) override;

        void _accept(const AUCondition *condition) override;

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
        std::pair<Expr_ptr, Expr_ptr> compareCondition(const CompareCondition *element);

        template<typename T>
        void _acceptNary(const T *element);
    
        template<typename T>
        Expr_ptr copy_narry_expr(const T* el);
        
        template<typename T>
        std::shared_ptr<T> copy_compare_condition(const T *element);
    };
}

#endif //VERIFYPN_CTLVISITOR_H
