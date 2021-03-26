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

#ifndef VERIFYPN_VISIBLETRANSITIONVISITOR_H
#define VERIFYPN_VISIBLETRANSITIONVISITOR_H

#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/light_deque.h"

namespace LTL {
    class VisibleTransitionVisitor : public PetriEngine::PQL::Visitor {
    public:
        explicit VisibleTransitionVisitor(std::unique_ptr<bool[]> &visible)
                : _places(visible) {}

    protected:
        void _accept(const PetriEngine::PQL::NotCondition *element) override;

        void _accept(const PetriEngine::PQL::AndCondition *element) override;

        void _accept(const PetriEngine::PQL::OrCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanCondition *element) override;

        void _accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanCondition *element) override;

        void _accept(const PetriEngine::PQL::GreaterThanOrEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::EqualCondition *element) override;

        void _accept(const PetriEngine::PQL::NotEqualCondition *element) override;

        void _accept(const PetriEngine::PQL::DeadlockCondition *element) override;

        void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override;

        void _accept(const PetriEngine::PQL::UnfoldedIdentifierExpr *element) override;

        void _accept(const PetriEngine::PQL::LiteralExpr *element) override;

        void _accept(const PetriEngine::PQL::PlusExpr *element) override;

        void _accept(const PetriEngine::PQL::MultiplyExpr *element) override;

        void _accept(const PetriEngine::PQL::MinusExpr *element) override;

        void _accept(const PetriEngine::PQL::SubtractExpr *element) override;

        void _accept(const PetriEngine::PQL::ACondition *condition) override;

        void _accept(const PetriEngine::PQL::ECondition *condition) override;

        void _accept(const PetriEngine::PQL::GCondition *condition) override;

        void _accept(const PetriEngine::PQL::FCondition *condition) override;

        void _accept(const PetriEngine::PQL::XCondition *condition) override;

        void _accept(const PetriEngine::PQL::UntilCondition *condition) override;

    private:
        std::unique_ptr<bool[]> &_places;
    };
}

#endif //VERIFYPN_VISIBLETRANSITIONVISITOR_H
