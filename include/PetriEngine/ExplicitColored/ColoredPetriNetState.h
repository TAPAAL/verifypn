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

            void shrink() {
                marking.shrink();
            }

            ColoredPetriNetMarking marking;
            uint32_t lastTrans = 0;
            uint32_t lastBinding = 0;
        };
    }
}
#endif //COLOREDPETRINETSTATE_H