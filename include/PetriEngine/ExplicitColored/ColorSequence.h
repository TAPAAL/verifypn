#ifndef MULTI_SET_H
#define MULTI_SET_H

#include "AtomicTypes.h"
#include <vector>
#include "ExplicitColorTypes.h"
#include <ostream>

namespace PetriEngine::ExplicitColored {
    class ColorSequence {
    public:
        ColorSequence(const ColorSequence&) = default;
        ColorSequence(ColorSequence&&) = default;
        ColorSequence& operator=(const ColorSequence&) = default;
        ColorSequence& operator=(ColorSequence&&) = default;
        ColorSequence(const std::vector<Color_t>& sequence, const ColorType& type) :
        ColorSequence(sequence, type.basicColorSizes, type.colorSize) {}
        ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes, const uint32_t totalSize) {
            //Essentially reverse binding generator to generate unique id for product color
            uint64_t result = 0;
            auto interval = totalSize;
            for (size_t i = 0; i < sequence.size(); ++i) {
                const auto elementColor = sequence[i];
                const auto elementSize = sizes[i];
                result += interval / elementSize * elementColor;
                interval /= elementSize;
            }
            encodedValue = result;
        }
        ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes) :
        ColorSequence(sequence, sizes, getTotalSize(sizes)) {}

        explicit ColorSequence(const Color_t encodedColor) : encodedValue(encodedColor) {}

        Color_t encodedValue;

        //Operators might need typechecking if something changes
        bool operator<(const ColorSequence& other) const {
            return encodedValue < other.encodedValue;
        }
        bool operator==(const ColorSequence& other) const {
            return encodedValue == other.encodedValue;
        }

        bool operator!=(const ColorSequence& other) const {
            return encodedValue != other.encodedValue;
        }

        static Color_t getTotalSize(const std::vector<Color_t>& sizes) {
            uint32_t totalSize = 1;
            for (auto& element : sizes) {
                totalSize *= element;
            }
            return totalSize;
        }


        std::vector<Color_t> decode(const std::vector<Color_t>& colorSizes, Color_t totalSize) const {
            std::vector<Color_t> rv;
            auto interval = totalSize;
            for (const auto colorSize : colorSizes) {
                const auto size = colorSize;
                interval /= size;
                rv.push_back((encodedValue / interval) % size);
            }
            return rv;
        }

        friend std::ostream& operator<<(std::ostream& os, const ColorSequence& colorSequence) {
            os << colorSequence.encodedValue;
            return os;
        }
    };
}

#endif