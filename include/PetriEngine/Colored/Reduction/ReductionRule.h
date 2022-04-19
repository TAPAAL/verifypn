/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_REDUCTIONRULE_H
#define VERIFYPN_REDUCTIONRULE_H

#include <vector>
#include <cstdint>
#include <string>
#include <PetriEngine/PQL/ColoredUseVisitor.h>

#include "PetriEngine/PQL/ColoredUseVisitor.h"

namespace PetriEngine::Colored::Reduction {
    class ColoredReducer;

    enum QueryType {
        Reach, LTL, CTL
    };

    class ReductionRule {
    public:
        virtual std::string name() = 0;

        virtual bool isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const = 0;

        uint32_t applications() {
            return _applications;
        }

        virtual bool apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery, QueryType queryType,
                           bool preserveLoops, bool preserveStutter) = 0;

    protected:
        uint32_t _applications = 0;
    };
}


#endif //VERIFYPN_REDUCTIONRULE_H
