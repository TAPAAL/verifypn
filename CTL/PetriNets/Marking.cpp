#include <string.h>
#include <sstream>
#include <iostream>

#include "Marking.h"

using namespace PetriEngine;

namespace PetriNets {
    Marking::Marking(MarkVal *t_marking)
    {
        this->setMarking(t_marking);
    }

    void Marking::copyMarking(const Marking &marking, uint32_t nplaces)
    {
        auto m = (MarkVal*) malloc(sizeof(MarkVal) * nplaces);
        for(uint32_t i = 0; i < nplaces; i++){
            m[i] = marking[i];
        }
        this->setMarking(m);
    }
}

