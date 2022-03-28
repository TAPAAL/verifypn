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
        std::string name() override { return "AtomicPreAgglomeration"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override;

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;

    private:
        uint32_t explosion_limiter = 1;
    };
}

#endif //VERIFYPN_REDRULEIDENTITY_H
