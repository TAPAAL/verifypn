//
// Created by emil on 2/18/25.
//
#ifndef COLOREDENCODER_H
#define COLOREDENCODER_H

#include "utils/structures/binarywrapper.h"
#include "ColoredPetriNetMarking.h"

namespace PetriEngine::ExplicitColored {
    class ColoredEncoder {
    public:
        typedef ptrie::binarywrapper_t scratchpad_t;
        explicit ColoredEncoder(std::vector<ColoredPetriNetPlace>& places) : _places(places) {
            const size_t bytes = 2 * sizeof(uint32_t) + _places.size() * sizeof(uint32_t);
            _scratchpad = scratchpad_t(bytes * 8);
        }

        size_t encode (const uint32_t* data, unsigned char type){
          return 5;
        }

        void decode(uint32_t* destination, const unsigned char* source) const{

        }

        [[nodiscard]] size_t size() const {
          return 5;
        }
    private:
        scratchpad_t _scratchpad;
        // uint32_t _places;
        std::vector<ColoredPetriNetPlace>& _places;
        void _writePlaces(const ColoredPetriNetMarking& data){

        }

        template<typename T>
        void _writeTokenCounts(size_t offset, const ColoredPetriNetMarking& data){
            for(size_t i = 0; i < data.markings.size(); ++i) {
                auto& marking = data.markings[i];
                auto colors = _places[i].colorType->size; //Maybe figure out what's up with size
                for (auto [color, count] : marking.counts()) {
                    T* dest = (T*)(&_scratchpad.raw()[offset + (color * sizeof(T))]);
                    *dest = count[i];
                }

            }
        }


    };
}

#endif //COLOREDENCODER_H
