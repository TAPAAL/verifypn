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

#include "LTL/Structures/GuardInfo.h"
#include "LTL/Simplification/SpotToPQL.h"
#include <spot/twa/formula2bdd.hh>
#include <spot/tl/formula.hh>

namespace LTL {
    std::vector<GuardInfo> getGuardInfo(const Structures::BuchiAutomaton &buchi)
    {
        std::vector<GuardInfo> state_guards;
        std::vector<AtomicProposition> aps(buchi.ap_info.size());
        std::transform(std::begin(buchi.ap_info), std::end(buchi.ap_info), std::begin(aps),
                       [](const std::pair<int, AtomicProposition> &pair) { return pair.second; });
        for (unsigned state = 0; state < buchi._buchi->num_states(); ++state) {
            state_guards.emplace_back(state, buchi._buchi->state_is_accepting(state));
            for (auto &e : buchi._buchi->out(state)) {
                auto formula = spot::bdd_to_formula(e.cond, buchi.dict);
                if (e.dst == state) {
                    state_guards.back().retarding = toPQL(formula, aps);
                } else {
                    state_guards.back().progressing.push_back(toPQL(formula, aps));
                }
            }
            if (!state_guards.back().retarding) {
                state_guards.back().retarding = std::make_shared<PetriEngine::PQL::BooleanCondition>(false);
            }
        }
        return state_guards;
    }
}