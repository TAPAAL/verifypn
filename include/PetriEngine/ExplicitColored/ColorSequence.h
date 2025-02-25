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
        explicit ColorSequence(const std::vector<Color_t>& sequence, const ColorType& type) :
        ColorSequence(sequence, type.basicColorSizes, type.colorSize) {}
        explicit ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes, const uint32_t totalSize) {
            //Essentially reverse binding generator to generate unique id for product color
            uint64_t result = 0;
            auto interval = totalSize;
            for (size_t i = 0; i < sequence.size(); i++) {
                const auto elementColor = sequence[i];
                const auto elementSize = sizes[i];
                result += interval / elementSize * elementColor;
                interval /= elementSize;
            }
            color = result;
        }
        explicit ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes) :
        ColorSequence(sequence, sizes, getTotalSize(sizes)) {}
        explicit ColorSequence(const Color_t color) : color(color) {}

        Color_t color;

        //Operators might need typechecking if something changes
        bool operator<(const ColorSequence& other) const {
            return color < other.color;
        }
        bool operator==(const ColorSequence& other) const {
            return color == other.color;
        }

        bool operator!=(const ColorSequence& other) const {
            return !(*this == other);
        }

        static Color_t getTotalSize(const std::vector<Color_t>& sizes) {
            uint32_t totalSize = 1;
            for (auto& element : sizes) {
                totalSize *= element;
            }
            return totalSize;
        }

        friend std::ostream& operator<<(std::ostream& os, const ColorSequence& colorSequence) {
            os << colorSequence.color;
            return os;
        }
    };
}

#endif