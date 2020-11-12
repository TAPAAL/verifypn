/*
 * File:   LTLToBuchi.h
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 23/09/2020
 */

#ifndef VERIFYPN_LTLTOBUCHI_H
#define VERIFYPN_LTLTOBUCHI_H

#include "PetriParse/QueryParser.h"
#include "PetriEngine/PQL/QueryPrinter.h"

#include <iostream>
#include <string>

#include <spot/tl/formula.hh>

namespace LTL {
    struct AtomicProposition {
        Condition_ptr expression;
        std::string text;
    };

    using APInfo = std::vector<AtomicProposition>;
    std::string toSpotFormat(const QueryItem &query);
    void toSpotFormat(const QueryItem &query, std::ostream &os);
    std::pair<spot::formula, APInfo> to_spot_formula(const Condition_ptr& query);

    class BuchiSuccessorGenerator;
    BuchiSuccessorGenerator makeBuchiAutomaton(const Condition_ptr &query);

}

#endif //VERIFYPN_LTLTOBUCHI_H
