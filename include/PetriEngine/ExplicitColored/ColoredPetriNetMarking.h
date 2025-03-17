#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include <cmath>
#include "vector"
#include "SequenceMultiSet.h"

namespace PetriEngine::ExplicitColored{
    struct ColoredPetriNetMarking{
        ColoredPetriNetMarking() = default;
        ColoredPetriNetMarking(const ColoredPetriNetMarking& marking) = default;
        ColoredPetriNetMarking(ColoredPetriNetMarking&&) = default;
        ColoredPetriNetMarking& operator=(const ColoredPetriNetMarking& marking) = default;
        ColoredPetriNetMarking& operator=(ColoredPetriNetMarking&&) = default;

        std::vector<CPNMultiSet> markings;

        bool operator==(const ColoredPetriNetMarking& other) const{
            return markings == other.markings;
        }

        bool operator!=(const ColoredPetriNetMarking& other) const{
            return !(*this == other);
        }

        [[nodiscard]] MarkingCount_t getPlaceCount(const uint32_t placeIndex) const {
            return markings[placeIndex].totalCount();
        }

        void shrink() {
            for (auto& place : markings) {
                place.shrink();
            }
        }
    };
}


#endif //COLOREDPETRINETMARKING_H