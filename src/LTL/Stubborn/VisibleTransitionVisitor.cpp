/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LTL/Stubborn/VisibleTransitionVisitor.h"

namespace LTL {
    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::NotCondition *element) {
        (*element)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::AndCondition *element) {
        for (auto &cond : *element) {
            cond->visit(*this);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::OrCondition *element) {
        for (auto &cond : *element) {
            cond->visit(*this);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        element->getExpr1()->visit(*this);
        element->getExpr2()->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        element->getExpr1()->visit(*this);
        element->getExpr2()->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::EqualCondition *element) {
        element->getExpr1()->visit(*this);
        element->getExpr2()->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        element->getExpr1()->visit(*this);
        element->getExpr2()->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::DeadlockCondition *element) {
        // no-op
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::CompareConjunction *element) {
        for (const auto &cons : *element) {
            _places[cons._place] = true;
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) {

    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) {
        _places[element->offset()] = true;
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::LiteralExpr *element) {

    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::PlusExpr *element) {
        for (const auto &pid : element->places()) {
            _places[pid.first] = true;
        }
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        for (const auto &pid : element->places()) {
            _places[pid.first] = true;
        }
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::MinusExpr *element) {
        (*element)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::ACondition *condition) {
        (*condition)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::ECondition *condition) {
        (*condition)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::GCondition *condition) {
        (*condition)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::FCondition *condition) {
        (*condition)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::XCondition *condition) {
        (*condition)[0]->visit(*this);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::UntilCondition *condition) {
        condition->getCond1()->visit(*this);
        condition->getCond2()->visit(*this);
    }

}