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

#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/neverclaim.hh>
#include <spot/twa/twagraph.hh>
#include <utility>
#include <memory>

namespace LTL {
    class BuchiSuccessorGenerator {
    public:
        explicit BuchiSuccessorGenerator(Structures::BuchiAutomaton automaton)
                : aut(std::move(automaton))
        {
            deleter = SuccIterDeleter{&aut};
        }

        void prepare(size_t state)
        {
            auto curstate = aut.buchi->state_from_number(state);
            succ = _succ_iter{aut.buchi->succ_iter(curstate), SuccIterDeleter{&aut}};
            succ->first();
        }

        bool next(size_t &state, bdd &cond)
        {
            if (!succ->done()) {
                auto dst = succ->dst();
                state = aut.buchi->state_number(dst);
                cond = succ->cond();
                succ->next();
                dst->destroy();
                return true;
            }
            return false;
        }

        [[nodiscard]] bool is_accepting(size_t state) const
        {
            return aut.buchi->state_is_accepting(state);
        }

        [[nodiscard]] size_t initial_state_number() const
        {
            return aut.buchi->get_init_state_number();
        }

        [[nodiscard]] Condition_ptr getExpression(size_t i) const
        {
            return aut.ap_info.at(i).expression;
        }

        [[nodiscard]] bool is_weak() const
        {
            return (bool) aut.buchi->prop_weak();
        }

        void output_buchi(const std::string& file, BuchiOutType type) {
            std::ofstream fs(file);
            switch (type) {
                case BuchiOutType::Dot:
                    spot::print_dot(fs, aut.buchi);
                    break;
                case BuchiOutType::HOA:
                    spot::print_hoa(fs, aut.buchi, "s");
                    break;
                case BuchiOutType::Spin:
                    spot::print_never_claim(fs, aut.buchi);
                    break;
            }
        }

    private:
        Structures::BuchiAutomaton aut;

        struct SuccIterDeleter {
            Structures::BuchiAutomaton *aut;

            void operator()(spot::twa_succ_iterator *iter) const
            {
                aut->buchi->release_iter(iter);
            }
        };

        SuccIterDeleter deleter{};

        using _succ_iter = std::unique_ptr<spot::twa_succ_iterator, SuccIterDeleter>;
        _succ_iter succ = nullptr;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
