/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/ArcVarMultisetVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRuleDeadPlacesAndTransitions.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

#include <queue>

namespace PetriEngine::Colored::Reduction {
    bool RedRuleDeadPlacesAndTransitions::apply(ColoredReducer &red, const std::vector<bool> &inQuery,
                                       QueryType queryType, bool preserveLoops, bool preserveStutter) {

        if (red.hasTimedOut()) return false;

        // Use pflags and bits to keep track of places that can increase or decrease their number of tokens
        const uint8_t CAN_INC = 0b01;
        const uint8_t CAN_DEC = 0b10;
        red._pflags.resize(red.placeCount(), 0);
        std::fill(red._pflags.begin(), red._pflags.end(), 0);

        // Use tflags to mark processed fireable transitions
        red._tflags.resize(red.transitionCount(), 0);
        std::fill(red._tflags.begin(), red._tflags.end(), 0);

        // Queue of potentially fireable transitions to process
        std::queue<uint32_t> queue;

        auto processIncPlace = [&](uint32_t p) {
            if ((red._pflags[p] & CAN_INC) == 0) {
                red._pflags[p] |= CAN_INC;
                const Place& place = red.places()[p];
                for (uint32_t t : place._post) {
                    if (red._tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processDecPlace = [&](uint32_t p) {
            if ((red._pflags[p] & CAN_DEC) == 0) {
                red._pflags[p] |= CAN_DEC;
                const Place& place = red.places()[p];
                for (uint32_t t : place._post) {
                    if (red._tflags[t] == 0)
                        queue.push(t);
                }
            }
        };

        auto processEnabled = [&](uint32_t t) {
            red._tflags[t] = 1;
            const Transition& tran = red.transitions()[t];
            // Find and process negative preset and positive postset
            uint32_t i = 0, j = 0;
            while (i < tran.input_arcs.size() && j < tran.output_arcs.size())
            {
                const Arc& inArc = tran.input_arcs[i];
                const Arc& outArc = tran.output_arcs[i];

                if (inArc.place < outArc.place) {
                    processDecPlace(outArc.place);
                    i++;
                } else if (inArc.place > outArc.place) {
                    processIncPlace(outArc.place);
                    j++;
                } else {
                    // There are both an in and an out arc to this place. Is the effect non-zero?
                    uint32_t in_weight = inArc.expr->weight();
                    uint32_t out_weight = outArc.expr->weight();

                    if (in_weight > out_weight) {
                        processDecPlace(inArc.place);
                    } else if (in_weight < out_weight) {
                        processIncPlace(outArc.place);
                    }

                    i++; j++;
                }
            }
            for ( ; i < tran.input_arcs.size(); i++) {
                processDecPlace(tran.input_arcs[i].place);
            }
            for ( ; j < tran.output_arcs.size(); j++) {
                processIncPlace(tran.output_arcs[j].place);
            }
        };

        // Process initially enabled transitions (ignoring color)
        for (uint32_t t = 0; t < red.transitionCount(); ++t) {
            const Transition& tran = red.transitions()[t];
            if (tran.skipped)
                continue;
            bool enabled = true;
            for (const Arc& inArc : tran.input_arcs) {
                if (inArc.expr->weight() > red.places()[inArc.place].marking.size()) {
                    enabled = false;
                    break;
                }
            }
            for (const Arc& inhibArc : red.inhibitorArcs()) {
                if (inhibArc.transition == t && inhibArc.inhib_weight <= red.places()[inhibArc.place].marking.size()) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Now we find the fixed point of dead places and transitions iteratively

        while (!queue.empty()) {
            if (red.hasTimedOut()) return false;

            uint32_t t = queue.front();
            queue.pop();
            if (red._tflags[t] == 1) continue;

            // Is t enabled?
            bool enabled = true;
            for (const Arc& inArc : red.transitions()[t].input_arcs) {
                if ((red._pflags[inArc.place] & CAN_INC) == 0 && inArc.expr->weight() > red.places()[inArc.place].marking.size()) {
                    enabled = false;
                    break;
                }
            }
            for (const Arc& inhibArc : red.inhibitorArcs()) {
                if (inhibArc.transition == t
                        && (red._pflags[inhibArc.place] & CAN_DEC) == 0
                        && inhibArc.inhib_weight <= red.places()[inhibArc.place].marking.size()) {
                    enabled = false;
                    break;
                }
            }
            if (enabled) {
                processEnabled(t);
            }
        }

        // Remove places that cannot increase nor decrease as well as unfireable transitions
        bool anyRemoved = false;
        for (uint32_t p = 0; p < red.placeCount(); ++p) {
            if (!red.places()[p].skipped && !inQuery[p] && red._pflags[p] == 0) {
                red.skipPlace(p);
                anyRemoved = true;
            }
        }
        for (uint32_t t = 0; t < red.transitionCount(); ++t) {
            if (!red.transitions()[t].skipped && red._tflags[t] == 0) {
                red.skipTransition(t);
                anyRemoved = true;
            }
        }
        if (anyRemoved) {
            _applications++;
            return true;
        }
        return false;
    }
}
