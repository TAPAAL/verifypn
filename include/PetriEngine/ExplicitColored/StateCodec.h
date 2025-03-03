#ifndef STATE_CODEC_H
#define STATE_CODEC_H
#include <vector>
#include <stdexcept>
namespace PetriEngine::ExplicitColored {
    template<typename ET = size_t, typename VT = size_t>
    class StateCodec {
    public:
        StateCodec() = default;
        explicit StateCodec(std::vector<VT> stateSizes) {
            _totalValues = 1;
            for (auto size : stateSizes) {
                _totalValues *= size;
            }
            auto interval = _totalValues;
            for (auto size : stateSizes) {
                interval /= size;
                decodeValues.push_back(interval);
                valueSizes.push_back(size);
            }
        }
        StateCodec(const StateCodec&) = default;
        StateCodec(StateCodec&&) = default;
        StateCodec& operator=(const StateCodec&) = default;
        StateCodec& operator=(StateCodec&&) = default;

        VT decode(ET encoded, size_t index) const {
            return (encoded / decodeValues[index]) % valueSizes[index];
        }

        ET getMax() const {
          return _totalValues;
        }

    private:
        std::vector<ET> decodeValues;
        std::vector<ET> valueSizes;
        size_t _totalValues = 0;
    };
}

#endif