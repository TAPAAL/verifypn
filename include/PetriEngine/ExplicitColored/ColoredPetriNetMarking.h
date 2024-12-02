#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include "vector"
#include "CPNMultiSet.h"
namespace PetriEngine{
    namespace ExplicitColored{
        struct ColoredPetriNetMarking{
            ColoredPetriNetMarking() = default;
            ColoredPetriNetMarking(const ColoredPetriNetMarking& marking) = default;
            ColoredPetriNetMarking(ColoredPetriNetMarking&&) = default;
            ColoredPetriNetMarking& operator=(const ColoredPetriNetMarking& marking) {
                auto vec = std::vector<CPNMultiSet>{};
                for (const auto & i : marking.markings){
                    vec.push_back(i);
                }
                markings = std::move(vec);
                return *this;
            };
            ColoredPetriNetMarking& operator=(ColoredPetriNetMarking&&) = default;

            std::vector<CPNMultiSet> markings;

            bool operator==(const ColoredPetriNetMarking& other) const{
                return markings == other.markings;
            }

            MarkingCount_t getPlaceCount(const uint32_t placeIndex) const {
                return markings[placeIndex].totalCount();
            }

            void stableEncode(std::ostream& out) const {
                for (const auto& marking : markings) {
                    marking.stableEncode(out);
                    out << ".";
                }
            }
        };
    }
}

#endif //COLOREDPETRINETMARKING_H