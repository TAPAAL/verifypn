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

#include "LTL/Stubborn/LTLEvalAndSetVisitor.h"
#include "PetriEngine/PQL/Evaluation.h"

namespace LTL {

    using namespace PetriEngine::PQL;

    void LTLEvalAndSetVisitor::_accept(AndCondition *element) {
        auto res = Condition::RTRUE;
        for (auto &c : *element) {
            Visitor::visit(this, c.get());
            auto r = c->getSatisfied();
            if (r == Condition::RFALSE) res = Condition::RFALSE;
            else if (r == Condition::RUNKNOWN && res != Condition::RFALSE) res = Condition::RUNKNOWN;
        }
        element->setSatisfied(res);
    }

    void LTLEvalAndSetVisitor::_accept(OrCondition *element) {
        auto res = Condition::RFALSE;
        for (auto &c : *element) {
            Visitor::visit(this, c.get());
            auto r = c->getSatisfied();
            if (r == Condition::RTRUE) res = Condition::RTRUE;
            else if (r == Condition::RUNKNOWN && res != Condition::RTRUE) res = Condition::RUNKNOWN;
        }
        element->setSatisfied(res);
    }
}
