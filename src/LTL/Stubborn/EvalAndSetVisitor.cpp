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

#include "LTL/Stubborn/EvalAndSetVisitor.h"
#include "PetriEngine/PQL/Evaluation.h"

namespace LTL {

    using namespace PetriEngine::PQL;

    void visit(EvalAndSetVisitor* visitor, PetriEngine::PQL::Condition* c)
    {
        // we pray that the compiler optimizes this
        auto* t = &typeid(*c);
        if      (t == &typeid(ACondition))
            visitor->accept(static_cast<ACondition*>(c));
        else if (t == &typeid(ECondition))
            visitor->accept(static_cast<ECondition*>(c));
        else if (t == &typeid(XCondition))
            visitor->accept(static_cast<XCondition*>(c));
        else if (t == &typeid(GCondition))
            visitor->accept(static_cast<GCondition*>(c));
        else if (t == &typeid(FCondition))
            visitor->accept(static_cast<FCondition*>(c));
        else if (t == &typeid(UntilCondition))
            visitor->accept(static_cast<UntilCondition*>(c));
        else if (t == &typeid(NotCondition))
            visitor->accept(static_cast<NotCondition*>(c));
        else if (t == &typeid(AndCondition))
            visitor->accept(static_cast<NotCondition*>(c));
        else if (t == &typeid(OrCondition))
            visitor->accept(static_cast<NotCondition*>(c));
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::ACondition *condition) {
        visit(this, condition->getCond().get());
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::ECondition *condition) {
        visit(this, condition->getCond().get());
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::XCondition *condition) {
        visit(this, condition->getCond().get());
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::GCondition *condition) {
        visit(this, condition->getCond().get());
        auto res = condition->getCond()->getSatisfied();
        if (res != Condition::RFALSE) res = Condition::RUNKNOWN;
        condition->setSatisfied(res);
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::FCondition *condition) {
        visit(this, condition->getCond().get());
        auto res = condition->getCond()->getSatisfied();
        if (res != Condition::RTRUE) res = Condition::RUNKNOWN;
        condition->setSatisfied(res);
    }

    void EvalAndSetVisitor::_accept(PetriEngine::PQL::UntilCondition *condition) {
        visit(this, condition->getCond1().get());
        visit(this, condition->getCond2().get());
        auto r2 = condition->getCond2()->getSatisfied();
        if (r2 != Condition::RFALSE) {
            condition->setSatisfied(r2);
            return;
        }
        auto r1 = condition->getCond1()->getSatisfied();
        if (r1 == Condition::RFALSE) {
            condition->setSatisfied(Condition::RFALSE);
            return;
        }
        condition->setSatisfied(Condition::RUNKNOWN);
    }

    void EvalAndSetVisitor::_accept(NotCondition *element) {
        visit(this, element->getCond().get());
        auto res = element->getCond()->getSatisfied();
        if (res != Condition::RUNKNOWN) res = res == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE;
        element->setSatisfied(res);
    }

    void EvalAndSetVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c : *element) {
            visit(this, c.get());
            auto r = c->getSatisfied();
            if (r == Condition::RFALSE) res = Condition::RFALSE;
            else if (r == Condition::RUNKNOWN && res != Condition::RFALSE) res = Condition::RUNKNOWN;
        }
        element->setSatisfied(res);
    }

    void EvalAndSetVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c : *element) {
            visit(this, c.get());
            auto r = c->getSatisfied();
            if (r == Condition::RTRUE) res = Condition::RTRUE;
            else if (r == Condition::RUNKNOWN && res != Condition::RTRUE) res = Condition::RUNKNOWN;
        }
        element->setSatisfied(res);
    }

    void EvalAndSetVisitor::_accept(LessThanCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(LessThanOrEqualCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(EqualCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(NotEqualCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(DeadlockCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(CompareConjunction *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }

    void EvalAndSetVisitor::_accept(BooleanCondition *element) {
        PetriEngine::PQL::evaluateAndSet(element, _context);
    }
}
