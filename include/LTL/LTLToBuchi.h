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
//#include "BuchiSuccessorGenerator.h"

#include <iostream>
#include <string>

namespace LTL {
    class BuchiSuccessorGenerator;
    std::string toSpotFormat(const QueryItem &query);
    void toSpotFormat(const QueryItem &query, std::ostream &os);

    BuchiSuccessorGenerator makeBuchiAutomaton(const Condition_ptr &query);

    struct AtomicProposition {
        Condition_constptr expression;
        std::string text;
    };

    using APInfo = std::vector<AtomicProposition>;
}

#endif //VERIFYPN_LTLTOBUCHI_H
