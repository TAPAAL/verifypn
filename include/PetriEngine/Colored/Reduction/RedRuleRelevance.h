/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULERELEVANCE_H
#define VERIFYPN_REDRULERELEVANCE_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleRelevance : public ReductionRule {
    public:
        std::string name() override { return "Relevance"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override;

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;

    private:
    };
}

#endif //VERIFYPN_REDRULERELEVANCE_H
