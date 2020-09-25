/*
 * File:   BuchiSuccessorGenerator.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 24/09/2020
 */

#ifndef VERIFYPN_BUCHISUCCESSORGENERATOR_H
#define VERIFYPN_BUCHISUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"

#include <spot/twa/twagraph.hh>
#include <utility>

class BuchiSuccessorGenerator {
public:
    BuchiSuccessorGenerator(spot::twa_graph_ptr automaton)
    : buchi(std::move(automaton))
    {
    }

    size_t get_initial_state() const {
        return buchi->get_init_state_number();
    }

    void prepare(size_t state)
    {
        auto curstate = buchi->state_from_number(state);
        succ = buchi->succ_iter(curstate);
    }

    bool next(size_t &state) {
        if (!succ->done()) {
            succ->next();
            state = buchi->state_number(succ->dst());
            return true;
        }
        return false;
    }

    bool is_accepting(size_t state) {
        return buchi->state_is_accepting(state);
    }

private:
    spot::twa_graph_ptr buchi;
    spot::twa_succ_iterator *succ;
};

#endif //VERIFYPN_BUCHISUCCESSORGENERATOR_H
