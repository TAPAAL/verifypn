/*
 * File:   TarjanSuccessorGenerator.cpp
 * Author: Nikolaj J. Ulrik <nikolaj@njulrik.dk>
 *
 * Created on 14/10/2020
 */

#include "LTL/TarjanSuccessorGenerator.h"

namespace LTL {

    bool TarjanSuccessorGenerator::next(LTL::Structures::ProductState &write, size_t &tindex) {
        SuccessorGenerator::next(write, tindex);
    }
}