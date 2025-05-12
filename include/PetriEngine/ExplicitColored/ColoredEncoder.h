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
        EMPTY,
    };

    enum TYPE_SIZE : unsigned char {
        EIGHT = 1,
        SIXTEEN = 2,
        THIRTYTWO = 4,
    };

    class ColoredEncoder {
    public:
        typedef ptrie::binarywrapper_t scratchpad_t;

        explicit ColoredEncoder(const std::vector<ColoredPetriNetPlace>& places) : _places(places),
            _placeSize(_convertToTypeSize(places.size())), _size(512) {
            for (const auto& place : _places) {
                _placeColorSize.push_back(_convertToTypeSize(place.colorType->colorSize));
            }

            _scratchpad = scratchpad_t(_size * 8);
        }

        ~ColoredEncoder() {
            _scratchpad.release();
        }

        //Encodes each place with its own encoding type, written as a prefix for each place
        size_t encode(const ColoredPetriNetMarking& marking) {
            size_t offset = 0;
            auto pid = 0;
            for (const auto& place : marking.markings) {
                const auto type = _getType(place, _places[pid].colorType->colorSize);
                _writeTypeSignature(type, offset);
                switch (type) {
                case TOKEN_COUNTS:
                    _writeTokenCounts(place, _places[pid].colorType->colorSize, offset);
                    break;
                case PLACE_TOKEN_COUNT:
                    _writePlaceTokenCounts(place, _placeColorSize[pid], offset);
                    break;
                case EMPTY:
                    break;
                }
                ++pid;
            }
            if (offset > UINT16_MAX) {
                //If too big for representation partial statespace will be explored
                if (_fullStatespace) {
                    _biggestRepresentation = UINT16_MAX;
                    std::cout << "State with size: " << offset <<
                        " cannot be represented correctly, so full statespace is not explored " << std::endl;
                }
                _fullStatespace = false;
                return UINT16_MAX;
            }
            _biggestRepresentation = std::max(offset, _biggestRepresentation);
            return offset;
        }

        ColoredPetriNetMarking decode(const unsigned char* encoding) const {
            size_t offset = 0;
            ColoredPetriNetMarking marking{};
            marking.markings.reserve(_places.size());
            for (auto pid = 0; pid < _places.size(); ++pid) {
                const auto type = static_cast<ENCODING_TYPE>(_readFromEncoding(encoding, EIGHT, offset));
                CPNMultiSet placeMultiset;
                switch (static_cast<ENCODING_TYPE>(type)) {
                case PLACE_TOKEN_COUNT:
                    placeMultiset = _decodePlaceTokenCounts(encoding, _placeColorSize[pid], offset);
                    break;
                case TOKEN_COUNTS:
                    placeMultiset = _decodeTokenCounts(encoding, _places[pid].colorType->colorSize, offset);
                    break;
                case EMPTY:
                    break;
                default:
                    throw explicit_error{ExplicitErrorType::UNKNOWN_ENCODING};
                }
                marking.markings.push_back(std::move(placeMultiset));
            }
            return marking;
        }

        [[nodiscard]] const uchar* data() const {
            return _scratchpad.const_raw();
        }

        size_t getBiggestEncoding() const {
            return _biggestRepresentation;
        }

        [[nodiscard]] bool testEncodingDecoding(const ColoredPetriNetMarking& marking) {
            encode(marking);
            const auto decodedMarking = decode(this->data());
            if (decodedMarking != marking) {
                std::cout << "De/encoding is not equivalent" << std::endl;
                return false;
            }
            return true;
        }

        [[nodiscard]] bool isFullStatespace() const {
            return _fullStatespace;
        }

    private:
        scratchpad_t _scratchpad;
        const std::vector<ColoredPetriNetPlace>& _places;
        size_t _size = 0;
        size_t _biggestRepresentation = 0;
        TYPE_SIZE _placeSize;
        std::vector<TYPE_SIZE> _placeColorSize = {};
        bool _fullStatespace = true;

        //Writes the cardinality of each color in the place in order, including 0
        //Could possibly use bits to show whether a token is non-zero
        void _writeTokenCounts(const CPNMultiSet& place, const Color_t colorNum, size_t& offset) {
            const auto highestCountType = _convertToTypeSize(place.getHighestCount());
            _writeToPad(highestCountType, EIGHT, offset);
            auto placeIterator = place.counts().begin();
            for (size_t colorId = 0; colorId < colorNum; colorId++) {
                auto count = 0;
                if (placeIterator != place.counts().end() && placeIterator->first == colorId) {
                    count = placeIterator->second;
                    ++placeIterator;
                }
                _writeToPad(count, highestCountType, offset);
            }
        }

        //Writes the type used for writing count then the amount of different colored tokens in the place followed by color-count pairs for each color token in the place
        void _writePlaceTokenCounts(const CPNMultiSet& place, const TYPE_SIZE placeColorSize, size_t& offset) {
            const auto highestCount = place.getHighestCount();
            const auto placeCountSize = _convertToTypeSize(highestCount);
            auto colorNumIndex = offset;
            auto colorNum = 0;
            offset += placeColorSize;
            _writeToPad(placeCountSize, EIGHT, offset);

            //Puts every token id followed by token count
            for (auto [color, count] : place.counts()) {
                if (count == 0) {
                    continue;
                }
                colorNum += 1;
                _writeToPad(color, placeColorSize, offset);
                _writeToPad(count, placeCountSize, offset);
            }
            //Puts amount of different tokens in the place - calculated on the fly
            _writeToPad(colorNum, placeColorSize, colorNumIndex);
        }

        //Decides what encoding should be used for a place Not sure what good heuristics are here
        //Multiset needs to be shrunk before being encoded or edge case exists where 0 cardinality token impacts encoding
        [[nodiscard]] static ENCODING_TYPE _getType(const CPNMultiSet& multiset, const Color_t possibleColors) {
            const uint32_t tokens = multiset.counts().size();
            if (tokens == 0) {
                return EMPTY;
            }

            //Relatively sparse
            if (tokens * 2 <= possibleColors) {
                return PLACE_TOKEN_COUNT;
            }
            return TOKEN_COUNTS;
        }

        void _writeTypeSignature(const ENCODING_TYPE type, size_t& offset) {
            _writeToPad(type, EIGHT, offset);
        }

        //Getting bit count could potentially be done faster
        static TYPE_SIZE _convertToTypeSize(const size_t n) {
            const auto bitCount = n == 0 ? 1 : static_cast<uint32_t>(std::floor(std::log2(n)) + 1);
            if (bitCount <= 8) {
                return EIGHT;
            }
            if (bitCount <= 16) {
                return SIXTEEN;
            }
            return THIRTYTWO;
        }

        static CPNMultiSet _decodeTokenCounts(const uchar* encoding, const Color_t colorNum, size_t& offset) {
            CPNMultiSet multiset{};
            const auto placeCountSize = static_cast<TYPE_SIZE>(_readFromEncoding(encoding, EIGHT, offset));
            for (size_t colorId = 0; colorId < colorNum; colorId++) {
                const MarkingCount_t count = _readFromEncoding(encoding, placeCountSize, offset);
                if (count > 0) {
                    multiset.setCount(colorId, count);
                }
            }
            return multiset;
        }

        static CPNMultiSet _decodePlaceTokenCounts(const uchar* encoding, const TYPE_SIZE placeColorSize,
                                                   size_t& offset) {
            CPNMultiSet multiset{};
            const auto multisetCardinality = _readFromEncoding(encoding, placeColorSize, offset);
            const auto multisetCountSize = static_cast<TYPE_SIZE>(_readFromEncoding(encoding, EIGHT, offset));
            for (size_t color = 0; color < multisetCardinality; color++) {
                const Color_t colorId = _readFromEncoding(encoding, placeColorSize, offset);
                const MarkingCount_t count = _readFromEncoding(encoding, multisetCountSize, offset);
                if (count != 0) {
                    multiset.setCount(colorId, count);
                }
            }
            return multiset;
        }

        //Doubles size, there could be a better way to minimize the size still limiting the resizes
        void _resizeScratchpad() {
            _size = _size * 2;
            auto newScratchpad = scratchpad_t(_size * 8);
            newScratchpad.copy(_scratchpad, 0);
            _scratchpad = newScratchpad;
        }

        template <typename T>
        void _writeToPad(const T element, const TYPE_SIZE typeSize, size_t& offset) {
            if (offset + typeSize > _size) {
                if (_size == UINT16_MAX) {
                    return;
                }
                _resizeScratchpad();
            }
            switch (typeSize) {
            case EIGHT: {
                _scratchpad.raw()[offset] = static_cast<uint8_t>(element);
                break;
            }
            case SIXTEEN: {
                const auto dest16 = (uint16_t*)(&_scratchpad.raw()[offset]);
                dest16[0] = static_cast<uint16_t>(element);
                break;
            }
            case THIRTYTWO: {
                const auto dest32 = (uint32_t*)(&_scratchpad.raw()[offset]);
                dest32[0] = static_cast<uint32_t>(element);
                break;
            }
            default:
                throw explicit_error{ExplicitErrorType::UNKNOWN_ENCODING};
            }
            offset += typeSize;
        }

        [[nodiscard]] static uint32_t
        _readFromEncoding(const uchar* encoding, const TYPE_SIZE typeSize, size_t& offset) {
            if (offset + typeSize > UINT16_MAX) {
                //If encoding is too big then we decode to 0
                return 0;
            }
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
                throw explicit_error{ExplicitErrorType::UNKNOWN_ENCODING};
            }
            offset += typeSize;
            return result;
        }
    };
}

#endif //COLOREDENCODER_H
