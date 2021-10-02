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
#include "LTL/AlgorithmTypes.h"

#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/neverclaim.hh>

#include <utility>
#include <memory>

namespace LTL {
    class BuchiSuccessorGenerator {
    public:
        explicit BuchiSuccessorGenerator(Structures::BuchiAutomaton automaton)
                : aut(std::move(automaton)), self_loops(aut._buchi->num_states(), InvariantSelfLoop::UNKNOWN)
        {
            deleter = SuccIterDeleter{&aut};
        }

        void prepare(size_t state)
        {
            auto curstate = aut._buchi->state_from_number(state);
            succ = _succ_iter{aut._buchi->succ_iter(curstate), SuccIterDeleter{&aut}};
            succ->first();
        }

        bool next(size_t &state, bdd &cond)
        {
            if (!succ->done()) {
                auto dst = succ->dst();
                state = aut._buchi->state_number(dst);
                cond = succ->cond();
                succ->next();
                dst->destroy();
                return true;
            }
            return false;
        }

        [[nodiscard]] bool is_accepting(size_t state) const
        {
            return aut._buchi->state_is_accepting(state);
        }

        [[nodiscard]] size_t initial_state_number() const
        {
            return aut._buchi->get_init_state_number();
        }

        [[nodiscard]] PetriEngine::PQL::Condition_ptr getExpression(size_t i) const
        {
            return aut.ap_info.at(i).expression;
        }

        [[nodiscard]] bool is_weak() const
        {
            return (bool) aut._buchi->prop_weak();
        }

        size_t buchiStates() { return aut._buchi->num_states(); }

        Structures::BuchiAutomaton aut;

        struct SuccIterDeleter {
            Structures::BuchiAutomaton *aut;

            void operator()(spot::twa_succ_iterator *iter) const
            {
                aut->_buchi->release_iter(iter);
            }
        };


        bool has_invariant_self_loop(size_t state) {
            if (self_loops[state] != InvariantSelfLoop::UNKNOWN)
                return self_loops[state] == InvariantSelfLoop::TRUE;
            auto it = std::unique_ptr<spot::twa_succ_iterator>{
                    aut._buchi->succ_iter(aut._buchi->state_from_number(state))};
            for (it->first(); !it->done(); it->next()) {
                auto dest_id = aut._buchi->state_number(it->dst());
                bdd cond = it->cond();
                if (state == dest_id && cond == bddtrue) {
                    self_loops[state] = InvariantSelfLoop::TRUE;
                    return true;
                }
            }
            self_loops[state] = InvariantSelfLoop::FALSE;
            return false;
        }


        SuccIterDeleter deleter{};

        using _succ_iter = std::unique_ptr<spot::twa_succ_iterator, SuccIterDeleter>;
        _succ_iter succ = nullptr;
    private:

        enum class InvariantSelfLoop {
            TRUE, FALSE, UNKNOWN
        };
        std::vector<InvariantSelfLoop> self_loops;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
