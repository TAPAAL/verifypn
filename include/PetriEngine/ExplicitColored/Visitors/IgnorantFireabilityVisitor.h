#ifndef FIREABILITYEVALUATE_H
#define FIREABILITYEVALUATE_H

#include "PetriEngine/PQL/Evaluation.h"

namespace PetriEngine::PQL {
    int64_t fireabilityEvaluate(Expr *element, const EvaluationContext& context);
    Condition::Result fireabilityEvaluate(Condition *element, const EvaluationContext& context);

    class FireabilityEvaluateVisitor : public BaseEvaluationVisitor {
    public:
        explicit FireabilityEvaluateVisitor(const EvaluationContext& context) : BaseEvaluationVisitor(context) {}
    private:
        void _accept(SimpleQuantifierCondition *element) override;

        void _accept(EGCondition *element) override { _notSupported("EG"); };

        void _accept(AGCondition *element) override;

        void _accept(ControlCondition *element) override;

        void _accept(EFCondition *element) override;

        void _accept(AFCondition *element) override { _notSupported("AF"); };

        void _accept(ACondition *element) override { _notSupported("A"); };

        void _accept(ECondition *element) override { _notSupported("E"); };

        void _accept(AllPaths *element) override;

        void _accept(ExistPath *element) override;

        void _accept(FCondition *element) override { _notSupported("F"); };

        void _accept(GCondition *element) override { _notSupported("G"); };

        void _accept(UntilCondition *element) override { _notSupported("Until"); };

        void _accept(AndCondition *element) override;

        void _accept(OrCondition *element) override;

        void _accept(CompareConjunction *element) override;

        void _accept(NotEqualCondition *element) override;

        void _accept(EqualCondition *element) override;

        void _accept(LessThanCondition *element) override;

        void _accept(LessThanOrEqualCondition *element) override;

        void _accept(NotCondition *element) override;

        void _accept(BooleanCondition *element) override;

        void _accept(DeadlockCondition *element) override;

        void _accept(UpperBoundsCondition *element) override { _notSupported("UpperBoundsCondition"); };

        void _accept(UnfoldedUpperBoundsCondition *element) override { _notSupported("UnfoldedUpperBoundsCondition"); };

        void _accept(ShallowCondition *element) override;

        static void _notSupported(const std::string& type) {
            throw base_error("Not supported ", type);
        }
    };
}
#endif //FIREABILITYEVALUATE_H
