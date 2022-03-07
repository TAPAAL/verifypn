/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_SPOTTOPQL_H
#define VERIFYPN_SPOTTOPQL_H

#include "LTL/LTLToBuchi.h"
#include "PetriEngine/PQL/Expressions.h"
#include <spot/tl/formula.hh>

namespace LTL {
    /**
     * Transform a Spot formula back into the PQL expression tree form.
     * @param formula The formula to translate to PQL
     * @param apinfo List of atomic propositions in the formula. This should have been created by {@link LTL::to_spot_formula}.
     * @return A PQL formula equivalent to the passed spot formula.
     */
    PetriEngine::PQL::Condition_ptr toPQL(const spot::formula &formula, const APInfo &apinfo);

    /**
     * Simplify an LTL formula using Spot's formula simplifier. Throws if formula is not valid LTL.
     * @param formula The formula to simplify.
     * @return the simplified formula.
     */
    PetriEngine::PQL::Condition_ptr simplify(const PetriEngine::PQL::Condition_ptr &formula, BuchiOptimization optimization = BuchiOptimization::High, APCompression compression = APCompression::None);
}

#endif // VERIFYPN_SPOTTOPQL_H
