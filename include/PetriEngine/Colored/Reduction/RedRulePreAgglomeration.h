/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEIDENTITY_H
#define VERIFYPN_REDRULEIDENTITY_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRulePreAgglomeration : public ReductionRule {
    public:
        std::string name() override { return "Atomic Pre-Agglomeration (Colored)"; }

        bool canBeAppliedRepeatedly() override { return true; }
        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override;

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter, uint32_t explosion_limiter) override;
    };
}

#endif //VERIFYPN_REDRULEIDENTITY_H
