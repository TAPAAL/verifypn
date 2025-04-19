#ifndef VARIABLECONSTRAINT_H
#define VARIABLECONSTRAINT_H

#include <cstdint>
#include "../AtomicTypes.h"

namespace PetriEngine::ExplicitColored {
    struct VariableConstraint {
        uint32_t colorIndex;
        ColorOffset_t colorOffset;
        Place_t place;

        static VariableConstraint getTop() {
            return VariableConstraint{
                std::numeric_limits<uint32_t>::max(),
                0
            };
        }

        [[nodiscard]] bool isTop() const {
            return colorIndex == std::numeric_limits<Place_t>::max();
        }
    };
}

#endif //VARIABLECONSTRAINT_H
