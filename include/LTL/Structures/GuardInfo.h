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

#ifndef VERIFYPN_GUARDINFO_H
#define VERIFYPN_GUARDINFO_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/Structures/BuchiAutomaton.h"
#include "LTL/Simplification/SpotToPQL.h"

#include <vector>
#include <spot/twa/twagraph.hh>
#include <spot/twa/formula2bdd.hh>



namespace LTL {
    struct guard_info_t {

        struct guard_t {
            PetriEngine::PQL::Condition_ptr _condition;
            bdd _bdd;
            uint32_t _dest;

            explicit operator bool () {
                return _condition != nullptr;
            }
        };

        guard_info_t(size_t buchiState, bool isAccepting) : _buchi_state(buchiState), _is_accepting(isAccepting) {}

        size_t _buchi_state;
        guard_t _retarding;
        std::vector<guard_t> _progressing;
        bool _is_accepting;


        static std::vector<guard_info_t> from_automaton(const Structures::BuchiAutomaton &aut) {
            std::vector<guard_info_t> state_guards;
            std::vector<AtomicProposition> aps;
            aps.reserve(aut.ap_info().size());
            for(auto& [id, ap] : aut.ap_info())
                aps.emplace_back(ap);
            for (decltype(aut.buchi().num_states()) state = 0; state < aut.buchi().num_states(); ++state) {
                state_guards.emplace_back(state, aut.buchi().state_is_accepting(state));
                for (auto &e : aut.buchi().out(state)) {
                    auto formula = spot::bdd_to_formula(e.cond, aut.buchi().get_dict());
                    if (e.dst == state) {
                        state_guards.back()._retarding = guard_t{toPQL(formula, aps), e.cond, state};
                    } else {
                        state_guards.back()._progressing.push_back(guard_t{toPQL(formula, aps), e.cond, e.dst});
                    }
                }
                if (!state_guards.back()._retarding) {
                    state_guards.back()._retarding = guard_t{std::make_shared<PetriEngine::PQL::BooleanCondition>(false), bddfalse, state};
                }
            }
            return state_guards;
        }
    };
}

#endif //VERIFYPN_GUARDINFO_H
