//
// Created by waefwerf on 21/09/2020.
//

#ifndef VERIFYPN_QUERYPRINTER_H
#define VERIFYPN_QUERYPRINTER_H

#include "Visitor.h"

#include <iostream>
#include <string>

namespace PetriEngine {
    namespace PQL {
        class QueryPrinter : public Visitor {
        public:
            QueryPrinter(std::ostream &os = std::cout)
                    : os(os) {}

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
            std::ostream &os;

            void _accept(const LogicalCondition *element, const std::string &op);

            void _accept(const CompareCondition *element, const std::string &op);

            void _accept(const NaryExpr *element, const std::string &op);
        };
    };
};

#endif //VERIFYPN_QUERYPRINTER_H
