/*
 * File:   BuchiSuccessorGenerator.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 24/09/2020
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
#ifdef PRINTF_DEBUG
                std::cerr << "buchi state " << state << std::endl;
#endif
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

    private:
        Structures::BuchiAutomaton aut;
        std::unique_ptr<spot::twa_succ_iterator> succ;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
