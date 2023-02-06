/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEPREAGGLOMERATION_H
#define VERIFYPN_REDRULEPREAGGLOMERATION_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRulePreAgglomeration : public ReductionRule {
    public:
        std::string name() override { return "AtomicPreAgglomeration"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override;

        bool apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, QueryType queryType,
                   bool preserveLoops, bool preserveStutter) override;

    private:
        uint32_t explosion_limiter = 5;
        static std::pair<bool, bool> _prodHangingGuardVar(ColoredReducer& red, uint32_t pid, const std::vector<uint32_t>& originalProducers);
    };
}

#endif //VERIFYPN_REDRULEPREAGGLOMERATION_H
