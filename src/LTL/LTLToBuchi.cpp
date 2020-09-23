/*
 * File:   LTLToBuchi.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 23/09/2020
 */

#include "LTL/LTLToBuchi.h"

namespace LTL {
    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotCondition *element) {
        os << "(! ";
        (*element)[0]->visit(*this);
        os << ")";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AndCondition *element) {
        QueryPrinter::_accept(element, "&&");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::OrCondition *element) {
        QueryPrinter::_accept(element, "||");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::CompareConjunction *element) {
        QueryPrinter::_accept(element);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::FireableCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::GreaterThanCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EqualCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::BooleanCondition *element) {
        os << (element->value ? "1" : "0");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LiteralExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::PlusExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MinusExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::IdentifierExpr *element) {
        os << "\"";
        QueryPrinter::_accept(element);
        os << "\"";
    }
}