#ifndef BINDING_H
#define BINDING_H

#include "AtomicTypes.h"
#include <vector>

namespace PetriEngine {
    namespace ExplicitColored {
        struct Binding
        {
            Color_t getValue(Variable_t variable);

        private:
            std::vector<Color_t> _values;
        };
    }
}

#endif