#ifndef COLORED_MARKING_SET_H
#define COLORED_MARKING_SET_H

#include "ColoredPetriNetMarking.h"
#include <unordered_set>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

namespace PetriEngine {
    namespace ExplicitColored {
        class ColoredMarkingSet {
        public:
            void add(const ColoredPetriNetMarking &marking) {
                std::stringstream stream;
                marking.stableEncode(stream);
                _set.insert(stream.str());
            }

            bool contains(const ColoredPetriNetMarking &marking) {
                std::stringstream stream;
                marking.stableEncode(stream);
                return _set.find(stream.str()) != _set.end();
            }

            size_t size() {
                return _set.size();
            }
        private:
            std::unordered_set<std::string> _set;
        };
    }
}
#endif
