#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include "vector"
#include "MultiSet.h"
namespace PetriEngine{
    namespace ExplicitColored{
        struct ColoredPetriNetMarking{
            std::vector<CPNMultiSet> markings;

            bool operator==(ColoredPetriNetMarking& other){
                return markings == other.markings;
            }

            MarkingCount_t getPlaceCount(uint32_t placeIndex) const {
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