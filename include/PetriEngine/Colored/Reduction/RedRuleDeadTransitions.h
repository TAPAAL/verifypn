/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEDEADTRANSITIONS_H
#define VERIFYPN_REDRULEDEADTRANSITIONS_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleDeadTransitions : public ReductionRule {
    public:
        std::string name() override { return "DeadTransitions"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override { return true; }

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;
    };
}

#endif //VERIFYPN_REDRULEDEADTRANSITIONS_H
