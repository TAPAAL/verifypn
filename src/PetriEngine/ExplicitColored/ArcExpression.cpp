#include "PetriEngine/ExplicitColored/ArcExpression.h"

namespace PetriEngine {
    namespace ExplicitColored {
        ArcExpression::ArcExpression(Colored::GuardExpression_ptr guardExpression, std::shared_ptr<Colored::ColorTypeMap> colorTypeMap)
            : _guardExpression(std::move(guardExpression)), _colorTypeMap(std::move(colorTypeMap)) {
        }
    }
}