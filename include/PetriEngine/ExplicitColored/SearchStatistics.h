#ifndef SEARCH_STATISTICS_H
#define SEARCH_STATISTICS_H

#include <cstdint>

namespace PetriEngine::ExplicitColored {
    struct SearchStatistics {
        uint32_t passedCount = 0;
        uint32_t checkedStates = 0;
        uint32_t endWaitingStates = 0;
        uint32_t peakWaitingStates = 0;
        uint32_t exploredStates = 0;
    };
}

#endif