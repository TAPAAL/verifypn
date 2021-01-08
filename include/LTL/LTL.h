
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
    inline auto to_string(Algorithm alg) {
        switch (alg) {
            case Algorithm::NDFS:
                return "NDFS";
            case Algorithm::RandomNDFS:
                return "RNDFS";
            case Algorithm::Tarjan:
                return "TARJAN";
            default:
                std::cerr << "to_string: Invalid LTL Algorithm " << static_cast<int>(alg) << '\n';
                assert(false);
        }
    }

    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula);
}

#endif