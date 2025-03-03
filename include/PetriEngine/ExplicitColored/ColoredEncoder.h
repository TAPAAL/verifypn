//
// Created by emil on 2/18/25.
//
#ifndef COLOREDENCODER_H
#define COLOREDENCODER_H

#include "utils/structures/binarywrapper.h"
#include "ColoredPetriNet.h"
#include "ColoredPetriNetMarking.h"
#include "ExplicitErrors.h"

namespace PetriEngine::ExplicitColored {
    enum ENCODING_TYPE : unsigned char {
        TOKEN_COUNTS,
        PLACE_TOKEN_COUNT,
        NOT_SET,
    };

    enum TYPE_SIZE : unsigned char {
        EIGHT = 1,
        SIXTEEN = 2,
        THIRTYTWO = 4,
        UNKNOWN
    };

    class ColoredEncoder {
    public:
        typedef ptrie::binarywrapper_t scratchpad_t;
        explicit ColoredEncoder(const std::vector<ColoredPetriNetPlace>& places) : _places(places), _placeSize(convertToTypeSize(places.size())) {
            //Arbitrary number will get resized
            _size = 1000;

            for (const auto& place : _places) {
                _placeColorSize.push_back(convertToTypeSize(place.colorType->colorSize));
            }

            _scratchpad = scratchpad_t(_size * 8);
        }

        ~ColoredEncoder() {
            std::cout << "The biggest represented state was: " << _biggestRepresentation << " bytes" << std::endl;
            _scratchpad.release();
        }

        size_t encode (const ColoredPetriNetMarking& marking, ENCODING_TYPE type = NOT_SET){
            _scratchpad.zero();
            const auto countSize = convertToTypeSize(marking.getHighestCount());
            if (type == NOT_SET) {
                type = _getType(marking);
            }
            auto offset = _writeTypeSignature(type, countSize, 0);
            switch (type) {
                case TOKEN_COUNTS:
                    offset = _writeTokenCounts(marking, countSize, offset);
                    break;
                case PLACE_TOKEN_COUNT:
                    offset = _writePlaceTokenCounts(marking, countSize, offset);
                    break;
                default:
                    throw explicit_error{unknown_encoding};
            }

            if (offset > 65536) {
                std::cout << "State with size: " << offset << " cannot be represented correctly" << std::endl;
            }
            if (offset > _scratchpad.size()){
                auto newScratchpad = scratchpad_t((offset + 500) * 8);
                newScratchpad.copy(_scratchpad, 0);
                _scratchpad = newScratchpad;
                return encode(marking, type);
            }
            _biggestRepresentation = std::max(offset, _biggestRepresentation);
            return offset;
        }

        ColoredPetriNetMarking decode(const unsigned char* encoding) const{
            size_t offset = 0;
            const auto type = static_cast<ENCODING_TYPE>(_readFromEncoding(encoding, EIGHT, offset));
            const auto colorSize = static_cast<TYPE_SIZE>(_readFromEncoding(encoding, EIGHT, offset));
            switch (static_cast<ENCODING_TYPE>(type)) {
            case PLACE_TOKEN_COUNT:
                return _decodePlaceTokenCounts(encoding, colorSize, offset);
            case TOKEN_COUNTS:
                return _decodeTokenCounts(encoding, colorSize, offset);
            default:
                throw explicit_error{unknown_encoding};
            }
        }

        [[nodiscard]] size_t size() const {
          return _size;
        }

        [[nodiscard]] const uchar* data() const {
            return _scratchpad.const_raw();
        }

        [[nodiscard]] bool testEncodingDecoding(const ColoredPetriNetMarking& marking) {
            this->encode(marking, PLACE_TOKEN_COUNT);
            const auto placeTokenDecoded = decode(this->data());
            if (placeTokenDecoded != marking) {
                std::cout << "PLACE_TOKEN_COUNT is not en/de-coded correctly" << std::endl;
                return false;
            }
            this->encode(marking, TOKEN_COUNTS);
            const auto tokenCountsDecoded = decode(this->data());
            if (tokenCountsDecoded != marking) {
                std::cout << "TOKEN_COUNTS is not en/de-coded correctly" << std::endl;
                return false;
            }
            return true;
        }
    private:
        scratchpad_t _scratchpad;
        const std::vector<ColoredPetriNetPlace>& _places;
        size_t _size = 0;
        size_t _biggestRepresentation = 0;
        TYPE_SIZE _placeSize;
        std::vector<TYPE_SIZE> _placeColorSize = {};

        void _writePlaces(const ColoredPetriNetMarking& data){

        }

        //Writes the cardinality of each color in each place in order, including 0
        //Could possibly use bits to show whether a token is non-zero
        size_t _writeTokenCounts(const ColoredPetriNetMarking& data, const TYPE_SIZE countSize, size_t& offset){
            for(size_t i = 0; i < data.markings.size(); i++) {
                auto& placeMultiset = data.markings[i];
                const auto colors = _places[i].colorType->colorSize;
                auto placeIterator = placeMultiset.counts().begin();
                for (size_t colorId = 0; colorId < colors; colorId++) {
                    auto count = 0;
                    if (placeIterator != placeMultiset.counts().end() && placeIterator->first == colorId) {
                        count = placeIterator->second;
                        ++placeIterator;
                    }
                    _writeToPad(count, countSize, offset);
                }
            }
            return offset;
        }

        size_t _writePlaceTokenCounts(const ColoredPetriNetMarking& data, const TYPE_SIZE countSize, size_t& offset) {
            for(size_t pid = 0; pid < data.markings.size(); pid++) {
                auto& placeMultiset = data.markings[pid];
                if (placeMultiset.getHighestCount() == 0 && pid != data.markings.size() - 1) {
                    continue;
                }
                const auto placeColorSize = _placeColorSize[pid];
                //Puts place id
                _writeToPad(pid, _placeSize, offset);
                //Save index for inputting amount of different colors in place
                auto colorNumIndex = offset;
                auto colorNum = 0;
                offset += placeColorSize;

                //Puts every token id followed by token count
                for (auto [color, count] : placeMultiset.counts()) {
                    if (count == 0) {
                        continue;
                    }
                    colorNum += 1;
                    _writeToPad(color, placeColorSize, offset);
                    //Maybe use different count size for each multiset
                    _writeToPad(count, countSize, offset);
                }
                //Puts amount of different tokens in the place - calculated on the fly
                _writeToPad(colorNum, placeColorSize, colorNumIndex);
            }
            return offset;
        }

        ENCODING_TYPE _getType(const ColoredPetriNetMarking& marking) {
            return PLACE_TOKEN_COUNT;
            return TOKEN_COUNTS;
        }

        size_t _writeTypeSignature(const ENCODING_TYPE type, const TYPE_SIZE countSize, size_t offset) {
            _writeToPad(type, EIGHT, offset);
            _writeToPad(countSize, EIGHT, offset);
            return offset;
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

        ColoredPetriNetMarking _decodeTokenCounts(const uchar* encoding, const TYPE_SIZE countSize, size_t offset) const {
            ColoredPetriNetMarking marking{};
            for (const auto& place : _places) {
                const auto colorNum = place.colorType->colorSize;
                CPNMultiSet multiset{};
                for (size_t colorId = 0; colorId < colorNum; colorId++) {
                    const MarkingCount_t count = _readFromEncoding(encoding, countSize, offset);
                    if (count > 0) {
                        multiset.setCount(colorId, count);
                    }
                }
                marking.markings.push_back(std::move(multiset));
            }
            return marking;
        }

        ColoredPetriNetMarking _decodePlaceTokenCounts(const uchar* encoding, const TYPE_SIZE countSize, size_t offset) const {
            ColoredPetriNetMarking marking{};
            for (size_t pid = 0; pid < _places.size(); pid++) {
                CPNMultiSet multiset{};
                const auto nextPlace = _readFromEncoding(encoding, _placeSize, offset);
                while (pid < nextPlace) {
                    marking.markings.emplace_back();
                    ++pid;
                }
                const TYPE_SIZE placeColorSize = _placeColorSize[pid];
                const auto multisetCardinality = _readFromEncoding(encoding, placeColorSize, offset);
                for (size_t color = 0; color < multisetCardinality; color++) {
                    const Color_t colorId = _readFromEncoding(encoding, placeColorSize, offset);
                    const MarkingCount_t count = _readFromEncoding(encoding, countSize, offset);
                    if (count != 0) {
                        multiset.setCount(colorId, count);
                    }
                }
                marking.markings.emplace_back(std::move(multiset));
            }
            return marking;
        }

        //Doubles size, there could be a better way to minimize the size still limiting the resizes
        void _resizeScratchpad(const size_t offset) {
            _size = std::min( static_cast<int>(offset * 2), UINT16_MAX);
            auto newScratchpad = scratchpad_t(_size * 8);
            newScratchpad.copy(_scratchpad, 0);
            _scratchpad = newScratchpad;
        }

        template <typename T>
        void _writeToPad(const T element, const TYPE_SIZE typeSize, size_t& offset ) {
            if (offset + 4 > _size) {
                _resizeScratchpad(offset);
            }
            switch (typeSize) {
            case EIGHT:
                _scratchpad.raw()[offset] = static_cast<uint8_t>(element);
                break;
            case SIXTEEN:
                {
                    const auto dest16 = (uint16_t*)(&_scratchpad.raw()[offset]);
                    dest16[0] = static_cast<uint16_t>(element);
                }
                break;
            case THIRTYTWO:
                {
                    const auto dest32 = (uint32_t*)(&_scratchpad.raw()[offset]);
                    dest32[0] = static_cast<uint32_t>(element);
                }
                break;
            default:
                throw explicit_error{unknown_encoding};
            }
            offset += typeSize;
        }

        [[nodiscard]] static uint32_t _readFromEncoding(const uchar* encoding, const TYPE_SIZE typeSize, size_t& offset) {
            uint32_t result;
            switch (typeSize) {
            case EIGHT:
                result = static_cast<uint8_t>(encoding[offset]);
                break;
            case SIXTEEN:
                    result = *(uint16_t*)(&encoding[offset]);
                break;
            case THIRTYTWO:
                result = *(uint32_t*)(&encoding[offset]);
                break;
            default:
                throw explicit_error{unknown_encoding};
            }
            offset += typeSize;
            return result;
        }
    };
}

#endif //COLOREDENCODER_H
