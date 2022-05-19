/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDRULEPREEMPTIVEFIRING_H
#define VERIFYPN_REDRULEPREEMPTIVEFIRING_H

#include "ReductionRule.h"
#include "PetriEngine/Colored/ColoredNetStructures.h"

namespace PetriEngine::Colored::Reduction {
    class RedRulePreemptiveFiring : public ReductionRule {
    public:
        std::string name() override { return "PreemptiveFiring"; }

        bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const override {
            return !preserveStutter;
        }

        bool apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, QueryType queryType,
                   bool preserveLoops, bool preserveStutter) override;

        bool transition_can_produce_to_place(unsigned int t, uint32_t p, ColoredReducer &red, std::set<uint32_t> &already_checked) const;

        bool t_is_viable(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, uint32_t t, uint32_t p);
    };
}


#endif //VERIFYPN_REDRULEPREEMPTIVEFIRING_H
