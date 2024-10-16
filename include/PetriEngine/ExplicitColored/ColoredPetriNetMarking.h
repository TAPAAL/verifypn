#ifndef COLOREDPETRINETMARKING_H
#define COLOREDPETRINETMARKING_H

#include "vector"
#include "MultiSet.h"
namespace PetriEngine{
    namespace ExplicitColored{
        struct ColoredPetriNetMarking{
            std::vector<CPNMultiSet> markings;

            bool operator==(ColoredPetriNetMarking& other){
                return markings == other.markings;
            }
        };
    }
}

#endif //COLOREDPETRINETMARKING_H