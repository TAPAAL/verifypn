//
// Created by emil on 2/18/25.
//
#ifndef COLOREDENCODER_H
#define COLOREDENCODER_H

#include "utils/structures/binarywrapper.h"
#include "ColoredPetriNet.h"
#include "ColoredPetriNetMarking.h"

namespace PetriEngine::ExplicitColored {
    enum ENCODING_TYPE : unsigned char {
        TOKEN_COUNTS,
        PLACE_TOKEN_COUNT,
    };

    enum TYPE_SIZE : unsigned char {
        EIGHT,
        SIXTEEN,
        THIRTYTWO,
    };

    class ColoredEncoder {
    public:
        typedef ptrie::binarywrapper_t scratchpad_t;
        explicit ColoredEncoder(const std::vector<ColoredPetriNetPlace>& places) : _places(places) {
            //Not true
            size_t bytes = 1;
            for (const auto& place : _places) {
                bytes += place.colorType->colorSize * 4;
            }
            _scratchpad = scratchpad_t(bytes * 8);
        }

        ~ColoredEncoder() {
            std::cout << "The biggest represented state was: " << biggestRepresentation << " bytes" << std::endl;
            _scratchpad.release();
        }

        size_t biggestRepresentation = 0;

        size_t encode (const ColoredPetriNetMarking& marking){
            _scratchpad.zero();
            const auto type = _getType(marking);
            auto offset = _writeTypeSignature(type, 0);
            switch (type) {
                case TOKEN_COUNTS:
                    offset = _writeTokenCounts(offset, marking);
                case PLACE_TOKEN_COUNT:
                    offset = _writePlaceTokenCounts(offset, marking);
                break;
            }
            if (offset > 65536 || offset > _scratchpad.size()) {
                std::cout << "State with size: " << offset << " cannot be represented correctly" << std::endl;
            }
            biggestRepresentation = std::max(offset, biggestRepresentation);
            _size = offset;
            return offset;
        }

        void decode(uint32_t* destination, const unsigned char* source) const{
            throw base_error("Not implemented");
        }

        [[nodiscard]] size_t size() const {
          return _size;
        }

        [[nodiscard]] const uchar* data() const {
            return _scratchpad.const_raw();
        }
    private:
        scratchpad_t _scratchpad;
        const std::vector<ColoredPetriNetPlace>& _places;
        size_t _size = 0;

        void _writePlaces(const ColoredPetriNetMarking& data){

        }

        //Writes the cardinality of each color in each place in order, including 0
        size_t _writeTokenCounts(const size_t offset, const ColoredPetriNetMarking& data){
            size_t internalOffset = 0;
            auto* dest8 = &_scratchpad.raw()[offset];
            auto* dest16 = reinterpret_cast<uint16_t*>(&_scratchpad.raw()[offset]);
            auto* dest32 = reinterpret_cast<uint32_t*>(&_scratchpad.raw()[offset]);

            for(size_t i = 0; i < data.markings.size(); i++) {
                auto& placeMultiset = data.markings[i];
                const auto colors = _places[i].colorType->colorSize;
                const auto typeSize = convertToTypeSize(placeMultiset.getHighestCount());
                auto placeIterator = placeMultiset.counts().begin();
                for (size_t c = 0; c < colors; c++) {
                    auto count = 0;
                    if (placeIterator != placeMultiset.counts().end() && placeIterator->first == c) {
                        count = placeIterator->second;
                        ++placeIterator;
                    }
                    switch (typeSize){
                        case EIGHT:
                            dest8[internalOffset] = static_cast<uint8_t>(count);
                            internalOffset += 1;
                            break;
                        case SIXTEEN:
                            dest16[(internalOffset + 1) / 2] = static_cast<uint16_t>(count);
                            internalOffset += 2;
                            break;
                        case THIRTYTWO:
                            dest32[(internalOffset + 3) / 2] = static_cast<uint32_t>(count);
                            internalOffset += 4;
                            break;
                    }
                }
            }
            return internalOffset + offset;
        }

        size_t _writePlaceTokenCounts(const size_t offset, const ColoredPetriNetMarking& data) {
            size_t internalOffset = 0;
            auto* dest8 = &_scratchpad.raw()[offset];
            auto* dest16 = reinterpret_cast<uint16_t*>(&_scratchpad.raw()[offset]);
            auto* dest32 = reinterpret_cast<uint32_t*>(&_scratchpad.raw()[offset]);
            const auto markingsSize = convertToTypeSize(data.markings.size());
            for(size_t i = 0; i < data.markings.size(); i++) {
                auto& placeMultiset = data.markings[i];
                if (placeMultiset.getHighestCount() == 0) {
                    continue;
                }
                //Maybe also put information about where next place is
                switch (markingsSize) {
                case EIGHT:
                    dest8[internalOffset] = static_cast<uint8_t>(i);
                    internalOffset += 1;
                    break;
                case SIXTEEN:
                    dest16[internalOffset] = static_cast<uint16_t>(i);
                    internalOffset += 2;
                    break;
                case THIRTYTWO:
                    dest32[internalOffset] = static_cast<uint32_t>(i);
                    internalOffset += 4;
                    break;
                }
                const auto colorSize = convertToTypeSize(_places[i].colorType->colorSize);
                const auto countSize = convertToTypeSize(placeMultiset.getHighestCount());
                for (auto [color, count] : placeMultiset.counts()) {
                    switch (colorSize) {
                    case EIGHT:
                        dest8[internalOffset] = static_cast<uint8_t>(color);
                        internalOffset += 1;
                        break;
                    case SIXTEEN:
                        dest16[(internalOffset + 1) / 2] = static_cast<uint16_t>(color);
                        internalOffset += 2;
                        break;
                    case THIRTYTWO:
                        dest32[(internalOffset + 3) / 2] = static_cast<uint32_t>(color);
                        internalOffset += 4;
                        break;
                    }
                    switch (countSize) {
                    case EIGHT:
                        dest8[internalOffset] = static_cast<uint8_t>(count);
                        internalOffset += 1;
                        break;
                    case SIXTEEN:
                        dest16[(internalOffset + 1) / 2] = static_cast<uint16_t>(count);
                        internalOffset += 2;
                        break;
                    case THIRTYTWO:
                        dest32[(internalOffset + 3) / 2] = static_cast<uint32_t>(count);
                        internalOffset += 4;
                        break;
                    }
                }
            }
            return offset + internalOffset;
        }

        ENCODING_TYPE _getType(const ColoredPetriNetMarking& marking) {
            return PLACE_TOKEN_COUNT;
        }

        size_t _writeTypeSignature(const ENCODING_TYPE type, const size_t offset) {
            _scratchpad.raw()[offset] = static_cast<unsigned char>(type);
            return offset + sizeof(ENCODING_TYPE);
        }

        static TYPE_SIZE convertToTypeSize(const size_t n){
            const auto bitCount = n == 0 ? 1 : static_cast<uint64_t>(std::floor(std::log2(n)) + 1);
            if (bitCount <= 8) {
                return EIGHT;
            }
            if (bitCount <= 16) {
                return SIXTEEN;
            }
            return THIRTYTWO;
        }

        // template<typename T>
        // void _addToScratchpad(size_t offset, const T toAdd, const int bitsToAdd) {
        //     // (int)toAdd <<
        // }
    };
}

#endif //COLOREDENCODER_H
