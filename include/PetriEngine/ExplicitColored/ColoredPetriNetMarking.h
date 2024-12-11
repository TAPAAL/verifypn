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

            void shrink() {
                for (auto& place : markings) {
                    place.shrink();
                }
            }

            void stableEncode(std::ostream& out) const {
                for (const auto& marking : markings) {
                    for (const auto& pair : marking.counts()) {
                        if (pair.second > 0) {
                            out << pair.second << "'(";
                            for (auto c : pair.first.getSequence()) {
                                out << c << ",";
                            }
                            out << ")";
                        }
                    }
                    out << ".";
                }
            }
        };
    }
}

#endif //COLOREDPETRINETMARKING_H