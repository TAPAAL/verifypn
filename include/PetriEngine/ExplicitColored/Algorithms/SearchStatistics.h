#ifndef SEARCH_STATISTICS_H
#define SEARCH_STATISTICS_H

#include <cstdint>
#include <cstddef>

namespace PetriEngine::ExplicitColored {
    struct SearchStatistics {
        uint32_t exploredStates = 0;
        uint32_t endWaitingStates = 0;
        uint32_t peakWaitingStates = 0;
        uint32_t discoveredStates = 0;
        size_t biggestEncoding = 0;
    };
}

#endif