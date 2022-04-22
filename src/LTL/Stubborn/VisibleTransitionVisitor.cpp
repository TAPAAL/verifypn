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
        Visitor::visit(this, (*element)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::AndCondition *element) {
        for (auto &cond : *element) {
            Visitor::visit(this, cond);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::OrCondition *element) {
        for (auto &cond : *element) {
            Visitor::visit(this, cond);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        Visitor::visit(this, element->getExpr1());
        Visitor::visit(this, element->getExpr2());
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        Visitor::visit(this, element->getExpr1());
        Visitor::visit(this, element->getExpr2());
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::EqualCondition *element) {
        Visitor::visit(this, element->getExpr1());
        Visitor::visit(this, element->getExpr2());
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        Visitor::visit(this, element->getExpr1());
        Visitor::visit(this, element->getExpr2());
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
            Visitor::visit(this, expr);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        for (const auto &pid : element->places()) {
            _places[pid.first] = true;
        }
        for (const auto &expr : element->expressions()) {
            Visitor::visit(this, expr);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::MinusExpr *element) {
        Visitor::visit(this, (*element)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        for (const auto &expr : element->expressions()) {
            Visitor::visit(this, expr);
        }
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::ACondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::ECondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::GCondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::FCondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::XCondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::UntilCondition *condition) {
        Visitor::visit(this, condition->getCond1());
        Visitor::visit(this, condition->getCond2());
    }

    void VisibleTransitionVisitor::_accept(const PetriEngine::PQL::ReleaseCondition *condition) {
        Visitor::visit(this, condition->getCond1());
        Visitor::visit(this, condition->getCond2());
    }

}