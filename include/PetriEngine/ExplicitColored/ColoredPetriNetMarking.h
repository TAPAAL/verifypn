#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include <cmath>

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

            static ColoredPetriNetMarking constructFromEncoding(std::vector<uint8_t>& bytes, size_t size) {
                ColoredPetriNetMarking marking;
                auto cursor = bytes.cbegin();
                const auto end = bytes.cbegin() + size;
                while (cursor != end) {
                    CPNMultiSet placeMultiSet;
                    while (*cursor != 0 && cursor != end) {
                        MarkingCount_t count = decodeVarInt<MarkingCount_t>(cursor, end);
                        ColorSequence sequence;
                        while (*cursor != 0 && cursor != end) {
                            Color_t color = decodeVarInt<Color_t>(cursor, end);
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

            size_t compressedEncode(std::vector<uint8_t>& bytes) const {
                size_t cursor = 0;
                for (const auto& marking : markings) {
                    for (const auto& pair : marking.counts()) {
                        if (pair.second > 0) {
                            encodeVarInt(bytes, cursor, pair.second);
                            for (auto c : pair.first.getSequence()) {
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
                for (size_t i = 0; i < bytesNeeded; ++i) {
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
}

#endif //COLOREDPETRINETMARKING_H