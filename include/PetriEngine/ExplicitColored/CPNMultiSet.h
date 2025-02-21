#ifndef MULTI_SET_H
#define MULTI_SET_H

#include "AtomicTypes.h"
#include <vector>
#include <utils/errors.h>
#include "SequenceMultiSet.h"
#include "ExplicitColorTypes.h"

namespace PetriEngine::ExplicitColored {
    class ColorSequence {
    public:
        ColorSequence() {
            color = 0;
        };
        ColorSequence(const ColorSequence&) = default;
        ColorSequence(ColorSequence&&) = default;
        ColorSequence& operator=(const ColorSequence&) = default;
        ColorSequence& operator=(ColorSequence&&) = default;
        explicit ColorSequence(const std::vector<Color_t>& sequence, const ColorType& type) : ColorSequence(sequence, type.basicColorSizes, type.colorSize) {
            // //Essentially reverse binding generator to generate unique id for product color
            // uint64_t result = 0;
            // const auto& interval = type.colorSize;
            // for (size_t i = 0; i < sequence.size(); i++) {
            //     const auto color = sequence[i];
            //     const auto colorSize = type.basicColorSizes[i];
            //     result += interval / colorSize * (color - 1);
            // }
            // color = result;
        }

        explicit ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes, const uint32_t totalSize) {
            //Essentially reverse binding generator to generate unique id for product color
            uint64_t result = 0;
            auto interval = totalSize;
            for (size_t i = 0; i < sequence.size(); i++) {
                const auto color = sequence[i];
                const auto colorSize = sizes[i];
                result += interval / colorSize * color;
                interval /= colorSize;
            }
            color = result;
        }
        //Pretty bad
        explicit ColorSequence(const std::vector<Color_t>& sequence, const std::vector<uint32_t>& sizes) {
           uint32_t totalSize = 1;
            for (auto& element : sizes) {
                totalSize *= element;
            }
            uint64_t result = 0;
            const auto& interval = totalSize;
            for (size_t i = 0; i < sequence.size(); i++) {
                const auto color = sequence[i];
                const auto colorSize = sizes[i];
                result += interval / colorSize * color;
            }
            color = result;
        }

        explicit ColorSequence(const Color_t color) : color(color) {}

        Color_t color;

        //Operators might need typechecking if something changes
        bool operator<(const ColorSequence& other) const {
            return color < other.color;
        }
        //Operators might need typechecking if something changes
        bool operator==(const ColorSequence& other) const {
            return color == other.color;
        }

        bool operator!=(const ColorSequence& other) const {
            return !(*this == other);
        }

        // // Essentially reverse binding generator
        // uint64_t getColor() const {
        //     uint64_t result = 0;
        //     const auto& interval = type.colors;
        //     for (size_t i = 0; i < _sequence.size(); i++) {
        //         const auto color = _sequence[i];
        //         const auto colorSize = type.basicColorTypes[i]->colors;
        //         result += interval / colorSize * (color - 1);
        //     }
        //     return result;
        // }

        // [[nodiscard]] const std::vector<Color_t>& getSequence() const {
        //     return _sequence;
        // }

        friend std::ostream& operator<<(std::ostream& os, const ColorSequence& colorSequence) {
            os << colorSequence.color;
            return os;
        }
    private:
        // std::vector<Color_t> _sequence;
        // PetriEngine::ExplicitColored::ColorType& _type;
    };

    typedef SequenceMultiSet<ColorSequence> CPNMultiSet;
}

#endif