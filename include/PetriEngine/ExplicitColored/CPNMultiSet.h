#ifndef MULTI_SET_H
#define MULTI_SET_H

#include "AtomicTypes.h" 
#include <vector>
#include <utils/errors.h>
#include "SequenceMultiSet.h"

namespace PetriEngine::ExplicitColored
{
    class ColorSequence {
    public:
        ColorSequence() = default;
        ColorSequence(const ColorSequence&) = default;
        ColorSequence(ColorSequence&&) = default;
        ColorSequence& operator=(const ColorSequence&) = default;
        ColorSequence& operator=(ColorSequence&&) = default;
        explicit ColorSequence(std::vector<Color_t> colorSequence)
            : _sequence(std::move(colorSequence)) {}

        bool operator<(const ColorSequence& other) const {
            if (_sequence.size() != other._sequence.size()) {
                throw base_error("Cannot compare sequences with inconsistent cardinalities");
            }
            for (size_t i = 0; i < _sequence.size(); i++) {
                if (_sequence[i] < other._sequence[i]) {
                    return true;
                }

                if (_sequence[i] > other._sequence[i]) {
                    return false;
                }
            }
            return false;
        }

        bool operator==(const ColorSequence& other) const {
            if (_sequence.size() != other._sequence.size())
                return false;
            for (size_t i = 0; i < _sequence.size(); i++) {
                if (_sequence[i] != other._sequence[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const ColorSequence& other) const {
            return !(*this == other);
        }

        [[nodiscard]] const std::vector<Color_t>& getSequence() const {
            return _sequence;
        }

        std::vector<Color_t>& getSequence() {
            return _sequence;
        }

        friend std::ostream& operator<<(std::ostream& os, const ColorSequence& colorSequence) {
            for (const auto& color : colorSequence._sequence) {
                os << color << ",";
            }
            return os;
        }
    private:
        std::vector<Color_t> _sequence;
    };

    typedef SequenceMultiSet<ColorSequence> CPNMultiSet;
}


#endif