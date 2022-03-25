/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEPARALLELTRANSITIONS_H
#define VERIFYPN_REDRULEPARALLELTRANSITIONS_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleParallelTransitions : public ReductionRule {
    public:
        std::string name() override { return "ParallelTransitions"; }

        bool canBeAppliedRepeatedly() override { return false; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override {
            return !preserveStutter;
        }

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter, uint32_t explosion_limiter) override;
    };
}

#endif //VERIFYPN_REDRULEPARALLELTRANSITIONS_H
