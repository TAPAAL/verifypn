#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include <utility>

#include "ColoredPetriNetMarking.h"
namespace PetriEngine{
    namespace ExplicitColored{
        struct ColoredPetriNetState{

            ColoredPetriNetState() = default;
            ColoredPetriNetState(const ColoredPetriNetState& oldState) {
                lastBinding = oldState.lastBinding;
                lastTrans = oldState.lastBinding;
                marking = oldState.marking;
            };
            explicit ColoredPetriNetState(ColoredPetriNetMarking  marking) : marking(std::move(marking)) {};
            ColoredPetriNetState(ColoredPetriNetState&&) = default;
            ColoredPetriNetState& operator=(const ColoredPetriNetState&) = default;
            ColoredPetriNetState& operator=(ColoredPetriNetState&&) = default;

            ColoredPetriNetMarking marking = ColoredPetriNetMarking{};
            uint32_t lastTrans = 0;
            uint32_t lastBinding = 0;
        };
    }
}
#endif //COLOREDPETRINETSTATE_H