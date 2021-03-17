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

#ifndef VERIFYPN_VISIBILITYVISITOR_H
#define VERIFYPN_VISIBILITYVISITOR_H

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Visitor.h"
#include "LTL/Stubborn/AutomatonStubbornSet.h"

namespace LTL {
    class VisibilityVisitor : public PetriEngine::PQL::BaseVisitor {
    public:
        VisibilityVisitor(const AutomatonStubbornSet &stubSet) : stubSet(stubSet) {}

        bool foundVisible() const { return done; }
    protected:
        void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override;
    private:
        const AutomatonStubbornSet &stubSet;
        bool done = false;

        bool postSet(uint32_t place);

        bool preSet(uint32_t place);

        void prePostSet(uint32_t place);
    };
}


#endif //VERIFYPN_VISIBILITYVISITOR_H
