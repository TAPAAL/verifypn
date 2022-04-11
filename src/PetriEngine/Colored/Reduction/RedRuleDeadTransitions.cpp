/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/ArcVarMultisetVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRuleDeadTransitions.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleDeadTransitions::apply(ColoredReducer &red, const std::vector<bool> &inQuery,
                                      QueryType queryType, bool preserveLoops, bool preserveStutter) {

        bool continueReductions = false;
        const size_t numberofplaces = red.placeCount();
        for(uint32_t p = 0; p < numberofplaces; ++p)
        {
            if(red.hasTimedOut()) return false;
            Place place = red.places()[p];
            if(place.skipped) continue;
            if(place.inhibitor) continue;
            if(place._pre.size() > place._post.size()) continue;

            bool ok = true;
            // Check for producers without matching consumers first
            for(uint prod : place._pre)
            {
                // Any producer without a matching consumer blocks this rule
                Transition t = red.transitions()[prod];
                auto in = red.getInArc(p, t);
                if(in == t.input_arcs.end())
                {
                    ok = false;
                    break;
                }
            }

            if(!ok) continue;

            std::set<uint32_t> notenabled;
            // Out of the consumers, tally up those that are initially not enabled by place
            // Ensure all the enabled transitions that feed back into place are non-increasing on place.
            for(uint cons : place._post)
            {
                Transition t = red.transitions()[cons];
                auto in = red.getInArc(p, t);
                if(in->expr->weight() <= place.marking.size())
                {
                    // This branch happening even once means notenabled.size() != consumers.size()
                    auto out = red.getOutArc(t, p);
                    // Only increasing loops are not ok
                    if (out != t.output_arcs.end() && out->expr->weight() > in->expr->weight()) {
                        ok = false;
                        break;
                    }
                }
                else
                {
                    notenabled.insert(cons);
                }
            }

            if(!ok || notenabled.empty()) continue;

            bool skipplace = (notenabled.size() == place._pre.size()) && (inQuery[p] == 0);

            for(uint cons : notenabled) {
                red.skipTransition(cons);
            }

            if(skipplace) {
                red.skipPlace(p);
            }

            _applications++;
            continueReductions = true;

        }
        red.consistent();
        return continueReductions;
    }
}