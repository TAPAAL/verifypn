#ifndef POSSIBLEVALUES_H
#define POSSIBLEVALUES_H

#include <algorithm>
#include <set>
#include "../AtomicTypes.h"

namespace PetriEngine::ExplicitColored {
struct PossibleValues {
            explicit PossibleValues(std::vector<Color_t> colors)
                : colors(std::move(colors)), allColors(false) {}

            explicit PossibleValues(const std::set<Color_t>& colors)
                : colors(colors.begin(), colors.end()), allColors(false) {}

            static PossibleValues getAll() {
                PossibleValues rv(std::vector<Color_t> {});
                rv.allColors = true;
                return rv;
            }

            static PossibleValues getEmpty() {
                PossibleValues rv(std::vector<Color_t> {});
                rv.allColors = false;
                return rv;
            }

            void sort() {
                std::sort(colors.begin(), colors.end());
            }

            void intersect(const PossibleValues& other) {
                if (other.allColors) {
                    return;
                }
                if (allColors) {
                    colors = other.colors;
                    return;
                }
                std::vector<Color_t> newColors;
                std::set_intersection(
                    colors.cbegin(),
                    colors.cend(),
                    other.colors.cbegin(),
                    other.colors.cend(),
                    std::back_inserter(newColors)
                );
                colors = std::move(newColors);
            }

            void intersect(const std::set<Color_t>& other) {
                if (allColors) {
                    colors.clear();
                    colors.insert(colors.begin(), other.cbegin(), other.cend());
                    return;
                }

                std::vector<Color_t> newColors;
                std::set_intersection(
                    colors.cbegin(),
                    colors.cend(),
                    other.cbegin(),
                    other.cend(),
                    std::back_inserter(newColors)
                );
                colors = std::move(newColors);
            }
            std::vector<Color_t> colors;
            bool allColors;
        };
}
#endif //POSSIBLEVALUES_H
