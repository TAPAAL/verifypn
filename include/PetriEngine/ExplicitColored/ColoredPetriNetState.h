#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include "ColoredPetriNetMarking.h"
namespace PetriEngine{
    namespace ExplicitColored{

        struct ColoredPetriNetState{
            ColoredPetriNetState() = default;
            ColoredPetriNetState(const ColoredPetriNetState& oldState) = default;
            explicit ColoredPetriNetState(ColoredPetriNetMarking marking) : marking(std::move(marking)) {};
            explicit ColoredPetriNetState(ColoredPetriNetMarking marking, bool forRDFS) : marking(std::move(marking)), forRDFS(forRDFS) {};
            ColoredPetriNetState(ColoredPetriNetState&&) = default;

            ColoredPetriNetState& operator=(const ColoredPetriNetState&) = default;
            ColoredPetriNetState& operator=(ColoredPetriNetState&&) = default;

            ColoredPetriNetMarking marking;
            Transition_t lastTrans = 0;
            Binding_t lastBinding = 0;

            bool forRDFS = false;
            bool hasAdded = false;
        };
    }
}
#endif //COLOREDPETRINETSTATE_H