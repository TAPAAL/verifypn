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

#ifndef VERIFYPN_BUCHISUCCESSORGENERATOR_H
#define VERIFYPN_BUCHISUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "LTL/Structures/BuchiAutomaton.h"

#include <spot/twa/twagraph.hh>
#include <utility>
#include <memory>

namespace LTL {
    class BuchiSuccessorGenerator {
    public:
        explicit BuchiSuccessorGenerator(Structures::BuchiAutomaton automaton)
                : aut(std::move(automaton)) {
        }

        void prepare(size_t state) {
            auto curstate = aut.buchi->state_from_number(state);
            succ = std::unique_ptr<spot::twa_succ_iterator>{aut.buchi->succ_iter(curstate)};
            succ->first();
        }

        bool next(size_t &state, bdd &cond) {
            if (!succ->done()) {
                state = aut.buchi->state_number(succ->dst());
                cond = succ->cond();
                succ->next();
                return true;
            }
            return false;
        }

        [[nodiscard]] bool is_accepting(size_t state) const {
            return aut.buchi->state_is_accepting(state);
        }

        [[nodiscard]] size_t initial_state_number() const {
            return aut.buchi->get_init_state_number();
        }

        [[nodiscard]] Condition_ptr getExpression(size_t i) const {
            return aut.ap_info.at(i).expression;
        }

        [[nodiscard]] bool is_weak() const {
            return (bool) aut.buchi->prop_weak();
        }

    private:
        Structures::BuchiAutomaton aut;
        std::unique_ptr<spot::twa_succ_iterator> succ;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
