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

#include "PetriEngine/PQL/Evaluation.h"
#include "PetriEngine/PQL/Contexts.h"

namespace LTL {

    class LTLEvalAndSetVisitor : public PetriEngine::PQL::EvaluateAndSetVisitor {

    public:
        LTLEvalAndSetVisitor(const PetriEngine::PQL::EvaluationContext &context) : EvaluateAndSetVisitor(context) {}

    protected:

    protected:
        void _accept(PetriEngine::PQL::AndCondition *element) override;
        void _accept(PetriEngine::PQL::OrCondition *element) override;

    };

}


#endif //VERIFYPN_EVALANDSETVISITOR_H
