//
// Created by mathi on 21/04/2022.
//

#ifndef VERIFYPN_REDRULEPREEMPTIVEFIRING_H
#define VERIFYPN_REDRULEPREEMPTIVEFIRING_H

#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRulePreemptiveFiring : public ReductionRule  {
        public:
            std::string name() override { return "PreemptiveFiring"; }

            bool
            isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override { return true; }

            bool apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, QueryType queryType,
                       bool preserveLoops, bool preserveStutter) override;
        };
}


#endif //VERIFYPN_REDRULEPREEMPTIVEFIRING_H
