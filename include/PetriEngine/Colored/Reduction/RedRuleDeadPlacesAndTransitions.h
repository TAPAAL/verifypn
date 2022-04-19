/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEDEADPLACESANDTRANSITIONS_H
#define VERIFYPN_REDRULEDEADPLACESANDTRANSITIONS_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleDeadPlacesAndTransitions : public ReductionRule {
    public:
        std::string name() override { return "DeadPlacesAndTransitions"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override { return true; }

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;
    };
}

#endif //VERIFYPN_REDRULEDEADPLACESANDTRANSITIONS_H
