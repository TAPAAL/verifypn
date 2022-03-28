/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEPARALLELTRANSITIONS_H
#define VERIFYPN_REDRULEPARALLELTRANSITIONS_H

#include <PetriEngine/Colored/Expressions.h>
#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleParallelTransitions : public ReductionRule {
    public:
        std::string name() override { return "ParallelTransitions"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override {
            return !preserveStutter;
        }

        bool apply(ColoredReducer &red, const std::vector<bool> &inQuery, QueryType queryType, bool preserveLoops, bool preserveStutter) override;

    private:
        static void checkMult(uint32_t &fail, uint32_t &mult, const ArcExpression &small, const ArcExpression &big);
    };
}

#endif //VERIFYPN_REDRULEPARALLELTRANSITIONS_H
