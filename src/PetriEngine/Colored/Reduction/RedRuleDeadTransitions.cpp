/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/ArcVarMultisetVisitor.h>
#include <PetriEngine/Colored/BindingGenerator.h>
#include <PetriEngine/Colored/EvaluationVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRuleDeadTransitions.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleDeadTransitions::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                       QueryType queryType, bool preserveLoops, bool preserveStutter) {

        Colored::PartitionBuilder partition(red.transitions(), red.places());
        bool continueReductions = false;
        const size_t numberofplaces = red.placeCount();
        for (uint32_t p = 0; p < numberofplaces; ++p) {
            if (red.hasTimedOut()) return false;
            Place place = red.places()[p];
            if (place.skipped) continue;
            if (place.inhibitor) continue;
            if (place._pre.size() > place._post.size()) continue;

            bool ok = true;
            // Check for producers without matching consumers first
            for (uint prod: place._pre) {
                // Any producer without a matching consumer blocks this rule
                const Transition &t = red.transitions()[prod];
                auto in = red.getInArc(p, t);
                if (in == t.input_arcs.end()) {
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            std::set<uint32_t> notenabled;
            // Out of the consumers, tally up those that are initially not enabled by place
            // Ensure all the enabled transitions that feed back into place are non-increasing on place.
            for (uint cons: place._post) {
                const Transition &t = red.transitions()[cons];

                const auto &in = red.getInArc(p, t);
                const auto &out = red.getOutArc(t, p);

                //Cheap check
                //Check if we might be able to fire the transition
                if (in->expr->weight() <= place.marking.size()) {
                    //The transition produces more tokens than it consumes to current place
                    if (out != t.output_arcs.end() && out->expr->weight() > in->expr->weight()) {
                        ok = false;
                        break;
                    }
                }

                //slightly more expensive check
                uint32_t bindingCount = red.getBindingCount(t);
                if (bindingCount > 10000) {
                    ok = false;
                    continue;
                }
                //lets actually look at the tokens and see if a binding enables the arc to the transition
                if (markingEnablesInArc(place.marking, *in, t, partition, red.colors())) {
                    //If there is no output, continue as it clearly cannot have an increasing effect on the place
                    if (out == t.output_arcs.end()) {
                        continue;
                    }

                    // Only increasing loops are not ok
                    auto inSet = PetriEngine::Colored::extractVarMultiset(*in->expr);
                    auto outSet = PetriEngine::Colored::extractVarMultiset(*out->expr);

                    if ((inSet && outSet && (*inSet).isSubsetOf(*outSet)) ||
                        to_string(*out->expr) != to_string(*in->expr)) {
                        ok = false;
                        break;
                    }

                } else {
                    notenabled.insert(cons);
                }
            }

            if (!ok || notenabled.empty()) continue;

            bool skipplace = (notenabled.size() == place._pre.size() && !inQuery.isPlaceUsed(p));

            for (uint32_t cons: notenabled) {
                if (inQuery.isTransitionUsed(cons))
                    skipplace = false;
                else {
                    red.skipTransition(cons);
                    _applications++;
                    continueReductions = true;
                }
            }

            if (skipplace) {
                red.skipPlace(p);
            }
        }
        red.consistent();
        return continueReductions;
    }

    bool RedRuleDeadTransitions::markingEnablesInArc(Multiset &marking, const Arc &arc,
                                                     const Colored::Transition &transition,
                                                     PartitionBuilder &partition,
                                                     const ColorTypeMap &colors) const {
        assert(arc.input);
        NaiveBindingGenerator gen(transition, colors);
        for (const auto &binding: gen) {
            const ExpressionContext context{binding, colors, partition.partition()[arc.place]};
            const auto ms = EvaluationVisitor::evaluate(*arc.expr, context);
            if (ms.isSubsetOrEqTo(marking)) return true;
        }
        return false;
    }
}