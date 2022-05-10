/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/PartitionBuilder.h>
#include <PetriEngine/Colored/EvaluationVisitor.h>
#include <PetriEngine/Colored/BindingGenerator.h>
#include "PetriEngine/Colored/Reduction/RedRuleRedundantPlaces.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleRedundantPlaces::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                       QueryType queryType, bool preserveLoops, bool preserveStutter) {

        Colored::PartitionBuilder partition(red.transitions(), red.places());

        bool continueReductions = false;
        const size_t numberofplaces = red.placeCount();
        for (uint32_t p = 0; p < numberofplaces; ++p) {

            if (red.unskippedPlacesCount() <= 1) break;
            if (red.hasTimedOut()) return false;

            Place place = red.places()[p];
            if (place.skipped) continue;
            if (place.inhibitor) continue;
            if (place._pre.size() < place._post.size()) continue;
            if (inQuery.isPlaceUsed(p)) continue;

            bool ok = true;
            for (uint cons: place._post) {
                const Transition &transition = red.transitions()[cons];

                /*if (transition.guard) {
                    ok = false;
                    break;
                }*/

                uint32_t bindingCount = red.getBindingCount(transition);
                if (bindingCount > 10000) {
                    ok = false;
                    break;
                }

                const auto &outArc = red.getOutArc(transition, p);
                if (outArc == transition.output_arcs.end()) {
                    ok = false;
                    break;
                }

                const auto &inArc = red.getInArc(p, transition);

                ok = markingEnablesInArc(place.marking, *inArc, transition, partition, red.colors());

                if (!ok) break;

                auto inSet = PetriEngine::Colored::extractVarMultiset(*inArc->expr);
                auto outSet = PetriEngine::Colored::extractVarMultiset(*outArc->expr);
                if (!inSet || !outSet || !(*inSet).isSubsetOrEqTo(*outSet)) {
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            ++_applications;
            red.skipPlace(p);
            continueReductions = true;

        }
        red.consistent();
        return continueReductions;
    }

    bool RedRuleRedundantPlaces::markingEnablesInArc(Multiset &marking, const Arc &arc,
                                                     const Colored::Transition &transition,
                                                     PartitionBuilder &partition,
                                                     const ColorTypeMap &colors) const {
        assert(arc.input);

        NaiveBindingGenerator gen(transition, colors);
        for (const auto &binding: gen) {
            const ExpressionContext context{binding, colors, partition.partition()[arc.place]};
            const auto ms = EvaluationVisitor::evaluate(*arc.expr, context);
            if (!(ms.isSubsetOrEqTo(marking))) return false;
        }
        return true;
    }
}