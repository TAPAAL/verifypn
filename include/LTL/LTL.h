
#ifndef VERIFYPN_LTLALGORITHM_H
#define VERIFYPN_LTLALGORITHM_H

#include "LTL/LTLValidator.h"
#include "LTL/Algorithm/TarjanModelChecker.h"
#include "LTL/Algorithm/NestedDepthFirstSearch.h"
#include "LTL/Algorithm/RandomNDFS.h"
#include "LTL/Simplification/SpotToPQL.h"
#include "LTL/LTLToBuchi.h"

namespace LTL {
    enum class Algorithm {
        NDFS, RandomNDFS, Tarjan, None=-1
    };

    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula);
}

#endif