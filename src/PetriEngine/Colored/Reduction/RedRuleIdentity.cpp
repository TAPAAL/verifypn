/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/Reduction/RedRuleIdentity.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleIdentity::apply(ColoredReducer &red, const std::vector<bool> &inQuery,
                                bool preserveDeadlocks) {

        // Do nothing
        _applications = 1;

        return false;
    }
}