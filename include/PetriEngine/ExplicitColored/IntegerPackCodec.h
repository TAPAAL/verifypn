#ifndef STATE_CODEC_H
#define STATE_CODEC_H
#include <vector>
#include <stdexcept>
namespace PetriEngine::ExplicitColored {
    template<typename ET = size_t, typename VT = size_t>
    class IntegerPackCodec {
    public:
        IntegerPackCodec() = default;
        explicit IntegerPackCodec(const std::vector<VT>& stateSizes) {
            _totalValues = 1;
            for (auto size : stateSizes) {
                _totalValues *= size;
            }
            _decodeValues.push_back(_totalValues);
            auto interval = _totalValues;
            for (auto size : stateSizes) {
                interval /= size;
                _decodeValues.push_back(interval);
                _valueSizes.push_back(size);
            }
        }
        IntegerPackCodec(const IntegerPackCodec&) = default;
        IntegerPackCodec(IntegerPackCodec&&) = default;
        IntegerPackCodec& operator=(const IntegerPackCodec&) = default;
        IntegerPackCodec& operator=(IntegerPackCodec&&) = default;

        VT decode(ET encoded, size_t index) const {
            return encoded / _decodeValues[index + 1] % _valueSizes[index];
        }

        ET addToValue(ET current, size_t index, VT valueToAdd) const {
            return current + _decodeValues[index] / _valueSizes[index] * valueToAdd;
        }

        ET getMax() const {
          return _totalValues;
        }

    private:
        std::vector<ET> _decodeValues;
        std::vector<ET> _valueSizes;
        size_t _totalValues = 0;
    };
}

#endif