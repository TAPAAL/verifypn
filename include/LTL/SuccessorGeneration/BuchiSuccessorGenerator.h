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
#include "LTL/LTLOptions.h"

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
                : _aut(std::move(automaton)), _self_loops(_aut.buchi().num_states(), InvariantSelfLoop::UNKNOWN)
        {
            _deleter = SuccIterDeleter{&_aut};
        }

        void prepare(size_t state)
        {
            auto curstate = _aut.buchi().state_from_number(state);
            _succ = _succ_iter{_aut.buchi().succ_iter(curstate), SuccIterDeleter{&_aut}};
            _succ->first();
        }

        bool next(size_t &state, bdd &cond)
        {
            if (!_succ->done()) {
                auto dst = _succ->dst();
                state = _aut.buchi().state_number(dst);
                cond = _succ->cond();
                _succ->next();
                dst->destroy();
                return true;
            }
            return false;
        }

        [[nodiscard]] bool is_accepting(size_t state) const
        {
            return _aut.buchi().state_is_accepting(state);
        }

        [[nodiscard]] size_t initial_state_number() const
        {
            return _aut.buchi().get_init_state_number();
        }

        [[nodiscard]] PetriEngine::PQL::Condition_ptr get_expression(size_t i) const
        {
            return _aut.ap_info().at(i)._expression;
        }

        [[nodiscard]] bool is_weak() const
        {
            return (bool) _aut.buchi().prop_weak();
        }

        size_t buchi_states() const { return _aut.buchi().num_states(); }

        bool has_invariant_self_loop(size_t state) {
            if (_self_loops[state] != InvariantSelfLoop::UNKNOWN)
                return _self_loops[state] == InvariantSelfLoop::TRUE;
            auto it = std::unique_ptr<spot::twa_succ_iterator>{
                    _aut.buchi().succ_iter(_aut.buchi().state_from_number(state))};
            for (it->first(); !it->done(); it->next()) {
                auto dest_id = _aut.buchi().state_number(it->dst());
                bdd cond = it->cond();
                if (state == dest_id && cond == bddtrue) {
                    _self_loops[state] = InvariantSelfLoop::TRUE;
                    return true;
                }
            }
            _self_loops[state] = InvariantSelfLoop::FALSE;
            return false;
        }

        const Structures::BuchiAutomaton& automaton() const {
            return _aut;
        }


    private:
        struct SuccIterDeleter {
            Structures::BuchiAutomaton *_aut;

            void operator()(spot::twa_succ_iterator *iter) const
            {
                _aut->buchi().release_iter(iter);
            }
        };

        enum class InvariantSelfLoop {
            TRUE, FALSE, UNKNOWN
        };
        Structures::BuchiAutomaton _aut;
        std::vector<InvariantSelfLoop> _self_loops;
        SuccIterDeleter _deleter{};
        using _succ_iter = std::unique_ptr<spot::twa_succ_iterator, SuccIterDeleter>;
        _succ_iter _succ = nullptr;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
