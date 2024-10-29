#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include "ColoredPetriNetMarking.h"
namespace PetriEngine{
    namespace ExplicitColored{
        struct ColoredPetriNetState{
            ColoredPetriNetMarking marking;
            uint32_t lastTrans = 0;
            uint32_t lastBinding = 0;
        };
    }
}
#endif //COLOREDPETRINETSTATE_H