#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include "ColoredPetriNetMarking.h"
namespace PetriEngine{
    namespace ExplicitColored{

        struct ColoredPetriNetState{
            ColoredPetriNetState() = default;
            ColoredPetriNetState(const ColoredPetriNetState& oldState) = default;
            explicit ColoredPetriNetState(ColoredPetriNetMarking marking) : marking(std::move(marking)) {};
            ColoredPetriNetState(ColoredPetriNetState&&) = default;

            ColoredPetriNetState& operator=(const ColoredPetriNetState&) = default;
            ColoredPetriNetState& operator=(ColoredPetriNetState&&) = default;

            ColoredPetriNetMarking marking;
            uint32_t lastTrans = 0;
            uint32_t lastBinding = 0;

            bool forRDFS = false;
            bool hasAdded = false;
        };

        // struct ColoredPetriNetStateRandom : ColoredPetriNetState {
        //     ColoredPetriNetStateRandom() = default;
        //     ColoredPetriNetStateRandom(const ColoredPetriNetStateRandom& oldState) = default;
        //     explicit ColoredPetriNetStateRandom(const ColoredPetriNetMarking& marking) {
        //         this->marking = marking;
        //         this->maxTrans = this->lastTrans + 1;
        //     } ;
        //     explicit ColoredPetriNetStateRandom(const ColoredPetriNetMarking& marking, const size_t seed, const uint32_t transitions){
        //             this->marking = marking;
        //             auto rng = std::default_random_engine {seed};
        //             for (uint32_t i = 0; i < transitions; i++){
        //                 transitionOrder.push_back(i);
        //             }
        //             std::shuffle(transitionOrder.begin(),transitionOrder.end(),rng);
        //         };
        //     ColoredPetriNetStateRandom(ColoredPetriNetStateRandom&&) = default;
        //     ColoredPetriNetStateRandom& operator=(const ColoredPetriNetStateRandom&) = default;
        //     ColoredPetriNetStateRandom& operator=(ColoredPetriNetStateRandom&&) = default;
        //
        //
        //     uint32_t maxTrans = lastTrans;
        //     std::vector<uint32_t> transitionOrder;
        // };
    }
}
#endif //COLOREDPETRINETSTATE_H