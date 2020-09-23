/*
 * File:   LTLToBuchi.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 23/09/2020
 */

#ifndef VERIFYPN_LTLTOBUCHI_H
#define VERIFYPN_LTLTOBUCHI_H

#include "PetriEngine/PQL/QueryPrinter.h"

namespace LTL {
    /**
     * Formula serializer to SPOT-compatible syntax.
     */
    class FormulaToSpotSyntax : public PetriEngine::PQL::QueryPrinter {
    protected:
        void _accept(const PetriEngine::PQL::NotCondition *element) override;

        void _accept(const PetriEngine::PQL::AndCondition *element) override;

        void _accept(const PetriEngine::PQL::OrCondition *element) override;

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::EqualCondition *element) override;

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) override;

        void _accept(const PetriEngine::PQL::FireableCondition *element) override;

        void _accept(const PetriEngine::PQL::BooleanCondition *element) override;

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override;

        void _accept(const PetriEngine::PQL::PlusExpr *element) override;

        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override;

        void _accept(const PetriEngine::PQL::MinusExpr *element) override;

        void _accept(const PetriEngine::PQL::SubtractExpr *element) override;

        void _accept(const PetriEngine::PQL::IdentifierExpr *element) override;
    };
}

#endif //VERIFYPN_LTLTOBUCHI_H
