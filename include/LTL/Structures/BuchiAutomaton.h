/*
 * File:   BuchiAutomaton.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 25/09/2020
 */

#ifndef VERIFYPN_BUCHIAUTOMATON_H
#define VERIFYPN_BUCHIAUTOMATON_H

#include "LTL/LTLToBuchi.h"
#include <spot/twa/twagraph.hh>

namespace LTL::Structures {
    struct BuchiAutomaton {
        spot::twa_graph_ptr buchi;
        const std::unordered_map<int, AtomicProposition> ap_info;
    };
}

#endif //VERIFYPN_BUCHIAUTOMATON_H
