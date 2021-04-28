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

#ifndef VERIFYPN_EVALANDSETVISITOR_H
#define VERIFYPN_EVALANDSETVISITOR_H

#include "PetriEngine/PQL/MutatingVisitor.h"
#include "PetriEngine/PQL/Contexts.h"

namespace LTL {

    class EvalAndSetVisitor : public PetriEngine::PQL::MutatingVisitor {

    public:
        EvalAndSetVisitor(const PetriEngine::PQL::EvaluationContext &context) : _context(context) {}

    private:
        const PetriEngine::PQL::EvaluationContext &_context;
    protected:

    protected:
        void _accept(PetriEngine::PQL::ACondition *condition) override;

        void _accept(PetriEngine::PQL::ECondition *condition) override;

        void _accept(PetriEngine::PQL::GCondition *condition) override;

        void _accept(PetriEngine::PQL::FCondition *condition) override;

        void _accept(PetriEngine::PQL::XCondition *condition) override;

        void _accept(PetriEngine::PQL::UntilCondition *condition) override;

        void _accept(PetriEngine::PQL::NotCondition *element) override;

        void _accept(PetriEngine::PQL::AndCondition *element) override;

        void _accept(PetriEngine::PQL::OrCondition *element) override;

        void _accept(PetriEngine::PQL::LessThanCondition *element) override;

        void _accept(PetriEngine::PQL::LessThanOrEqualCondition *element) override;

        void _accept(PetriEngine::PQL::EqualCondition *element) override;

        void _accept(PetriEngine::PQL::NotEqualCondition *element) override;

        void _accept(PetriEngine::PQL::DeadlockCondition *element) override;

        void _accept(PetriEngine::PQL::CompareConjunction *element) override;

        void _accept(PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override;

        void _accept(PetriEngine::PQL::BooleanCondition *element) override;
    };

}


#endif //VERIFYPN_EVALANDSETVISITOR_H
