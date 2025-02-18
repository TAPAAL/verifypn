#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include <cmath>

#include "vector"
#include "CPNMultiSet.h"
#include "ExplicitErrors.h"

namespace PetriEngine::ExplicitColored{
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

        [[nodiscard]] MarkingCount_t getPlaceCount(const uint32_t placeIndex) const {
            return markings[placeIndex].totalCount();
        }

        void shrink() {
            for (auto& place : markings) {
                place.shrink();
            }
        }

        static ColoredPetriNetMarking constructFromEncoding(const std::vector<uint8_t>& bytes, const uint size) {
            ColoredPetriNetMarking marking;
            auto cursor = bytes.cbegin();
            const auto end = bytes.cbegin() + size;
            while (cursor != end) {
                CPNMultiSet placeMultiSet;
                while (*cursor != 0 && cursor != end) {
                    const auto count = decodeVarInt<MarkingCount_t>(cursor, end);
                    ColorSequence sequence;
                    while (*cursor != 0 && cursor != end) {
                        const auto color = decodeVarInt<Color_t>(cursor, end);
                        sequence.getSequence().push_back(color - 1);
                    }
                    ++cursor;
                    placeMultiSet.setCount(sequence, count);
                }
                ++cursor;
                marking.markings.push_back(std::move(placeMultiSet));
            }
            return marking;
        }

        size_t compressedEncode(std::vector<uint8_t>& bytes, bool& success) const {
            size_t cursor = 0;
            for (const auto& marking : markings) {
                for (const auto& [set, count] : marking.counts()) {
                    if (count > 0) {
                        encodeVarInt(bytes, cursor, count);
                        for (const auto c : set.getSequence()) {
                            encodeVarInt(bytes, cursor, c + 1);
                        }
                        if (bytes.size() < cursor + 1) {
                            bytes.resize(cursor + 1);
                        }
                        bytes[cursor++] = 0;
                    }
                }
                if (bytes.size() < cursor + 1) {
                    bytes.resize(cursor + 1);
                }
                bytes[cursor++] = 0;
            }
            if (cursor >= std::numeric_limits<uint16_t>::max()) {
                if (success) {
                    std::cout << "Too big for ptrie, not exploring full statespace" << std::endl;
                }
                success = false;
            }
            return cursor;
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
    private:
        template<typename T>
        static void encodeVarInt(std::vector<uint8_t>& out, size_t& cursor, T value) {
            const uint8_t bitCount = std::floor(std::log2(value)) + 1;
            const uint8_t bytesNeeded = bitCount / 7 + (bitCount % 7 != 0 ? 1 : 0);
            if (out.size() < cursor + bytesNeeded) {
                out.resize(cursor + bytesNeeded);
            }
            for (uint16_t i = 0; i < bytesNeeded; ++i) {
                uint8_t byte = value & 0x7F;
                if (i != bytesNeeded - 1) {
                    byte = byte | 0x80;
                }
                out[cursor++] = byte;
                value = value >> 7;
            }
        }

        template<typename T>
        static T decodeVarInt(std::vector<uint8_t>::const_iterator& cursor, const std::vector<uint8_t>::const_iterator end) {
            T out = 0;
            size_t bits = 0;
            while (cursor != end) {
                out |= static_cast<T>(*cursor & 0x7F) << bits;
                bits += 7;
                if (!(*cursor & 0x80)) {
                    break;
                }
                ++cursor;
            }
            ++cursor;
            return out;
        }
    };
}


#endif //COLOREDPETRINETMARKING_H