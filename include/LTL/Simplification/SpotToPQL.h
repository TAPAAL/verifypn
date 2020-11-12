/*
 * File:   SpotToPQL.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 12/11/2020
 */

#ifndef VERIFYPN_SPOTTOPQL_H
#define VERIFYPN_SPOTTOPQL_H

#include "PetriEngine/PQL/Expressions.h"
#include "LTL/LTLToBuchi.h"
#include <spot/tl/formula.hh>

namespace LTL {
PetriEngine::PQL::Condition_ptr toPQL(const spot::formula &formula, const APInfo &apinfo);

/**
 * Simplify an LTL formula using Spot's formula simplifier. Throws if formula is not valid LTL.
 * @param formula The formula to simplify.
 * @return the simplified formula.
 */
PetriEngine::PQL::Condition_ptr simplify(const PetriEngine::PQL::Condition_ptr& formula);
}

#endif // VERIFYPN_SPOTTOPQL_H
