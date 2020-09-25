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

namespace LTL {
    class BuchiSuccessorGenerator {
    public:
        BuchiSuccessorGenerator(Structures::BuchiAutomaton automaton)
                : aut(std::move(automaton)) {
        }

        size_t get_initial_state() const {
            return aut.buchi->get_init_state_number();
        }

        void prepare(size_t state) {
            auto curstate = aut.buchi->state_from_number(state);
            succ = aut.buchi->succ_iter(curstate);
        }

        bool next(size_t &state, bdd &cond) {
            if (!succ->done()) {
                succ->next();
                state = aut.buchi->state_number(succ->dst());
                cond = succ->cond();
                return true;
            }
            return false;
        }

        bool is_accepting(size_t state) {
            return aut.buchi->state_is_accepting(state);
        }

        Condition_constptr getExpression(size_t i) {
            return aut.ap_info[i].expression;
        }

    private:
        Structures::BuchiAutomaton aut;
        spot::twa_succ_iterator *succ;
    };
}
#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
