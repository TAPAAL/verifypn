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

#include "LTL/Stubborn/VisibilityVisitor.h"
/*
namespace LTL {
    void VisibilityVisitor::_accept(const PetriEngine::PQL::CompareConjunction *element)
    {
        if (done) {
            return;
        }
        for (const auto &cons : element->constraints()) {
            if (done) return;
            prePostSet(cons._place);
        }
    }

    void VisibilityVisitor::_accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element)
    {
        if (done) {
            return;
        }
        prePostSet(element->offset());
    }

    bool VisibilityVisitor::postSet(uint32_t place)
    {
        return false;
    }

    bool VisibilityVisitor::preSet(uint32_t place)
    {
        return false;
    }

    void VisibilityVisitor::prePostSet(uint32_t place)
    {
        for (uint32_t t = stubSet._places.get()[place].pre; t < stubSet._places.get()[place].post; t++) {
            auto &tr = stubSet._transitions[t];
            if (stubSet._stubborn[tr.index]) {
                done = true;
                return;
            }
        }
        for (uint32_t t = stubSet._places.get()[place].post; t < stubSet._places.get()[place+1].pre; t++) {
            auto &tr = stubSet._transitions[t];
            if (tr.direction < 0 && stubSet._stubborn[tr.index]) {
                done = true;
                return;
            }
        }
    }
}*/