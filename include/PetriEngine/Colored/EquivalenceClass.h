#ifndef PARTITION_H
#define PARTITION_H

#include "Intervals.h"
#include "Colors.h"

namespace PetriEngine {
    namespace Colored {
        struct EquivalenceClass {
            ColorType *_colorType;
            intervalTuple_t _colorIntervals;
            

        };

        struct EquivalenceVec{
            std::vector<EquivalenceClass> _equivalenceClasses;
            bool diagonal;
            bool full;
        };

    }
}

#endif /* PARTITION_H */