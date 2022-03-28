/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEPARALLELPLACES_H
#define VERIFYPN_REDRULEPARALLELPLACES_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleParallelPlaces : public ReductionRule {
    public:
        std::string name() override { return "ParallelPlaces"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override { return true; }

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;
    };
}

#endif //VERIFYPN_REDRULEPARALLELPLACES_H
