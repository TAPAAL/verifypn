//
// Created by mathi on 08/04/2022.
//

#ifndef VERIFYPN_REDRULEREDUNDANTPLACES_H
#define VERIFYPN_REDRULEREDUNDANTPLACES_H

#include "../PartitionBuilder.h"
#include "ReductionRule.h"

namespace PetriEngine::Colored::Reduction {
    class RedRuleRedundantPlaces : public ReductionRule {
    public:
        std::string name() override { return "RedundantPlaces"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override { return true; }

        bool apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, QueryType queryType,
                   bool preserveLoops, bool preserveStutter) override;

        bool markingEnablesInArc(Multiset &marking, const Arc &arc,
                                 const Colored::Transition &transition,
                                 PartitionBuilder &partition,
                                 const ColorTypeMap &colors) const;
    };
}


#endif //VERIFYPN_REDRULEREDUNDANTPLACES_H
