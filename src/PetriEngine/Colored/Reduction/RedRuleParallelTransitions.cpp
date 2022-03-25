/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/Reduction/RedRuleParallelTransitions.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleParallelTransitions::apply(ColoredReducer &red, const std::vector<bool> &inQuery,
                                           QueryType queryType, bool preserveLoops, bool preserveStutter,
                                           uint32_t explosion_limiter) {

        // Remove transitions which effect is k times another transitions effect

        bool continueReductions = false;

        auto &transitions = red.transitions();
        red._tflags.resize(transitions.size(), 0);
        std::fill(red._tflags.begin(), red._tflags.end(), 0);

        // Remove all empty transitions, except one if we have to preserve loops and stuttering
        bool hasOtherEmptyTrans = !preserveLoops && !preserveStutter;
        for (size_t t = 0; t < transitions.size(); t++) {
            auto &trans = transitions[t];
            if (!trans.skipped && trans.input_arcs.empty() && trans.output_arcs.empty() && (!trans.inhibited || hasOtherEmptyTrans)) {
                if (hasOtherEmptyTrans) {
                    _applications++;
                    red.skipTransition(t);
                }
                hasOtherEmptyTrans = true;
            }
        }

        for (auto &op: red.places()) {
            for (size_t outer = 0; outer < op._post.size(); outer++) {
                auto touter = op._post[outer];
                if (red.hasTimedOut()) return false;
                if (red._tflags[touter] != 0) continue;
                red._tflags[touter] = 1;
                const Transition &tout = transitions[touter];

                if (tout.skipped) continue;

                for (size_t inner = outer + 1; inner < op._post.size(); inner++) {
                    if (tout.skipped) break;

                    auto tinner = op._post[inner];
                    const Transition &tin = transitions[tinner];

                    if (tin.skipped) continue;

                    for (size_t swp = 0; swp < 2; swp++) {

                        if (red.hasTimedOut()) return false;

                        auto t1 = touter;
                        auto t2 = tinner;
                        if (swp == 1) std::swap(t1, t2);

                        const Transition &trans1 = transitions[t1];
                        const Transition &trans2 = transitions[t2];

                        if (trans1.output_arcs.size() != trans2.output_arcs.size()) break;
                        if (trans1.input_arcs.size() != trans2.input_arcs.size()) break;

                        if ((trans1.guard == nullptr) != (trans2.guard == nullptr)) break;
                        if (trans1.guard != nullptr && trans2.guard != nullptr &&
                            to_string(*trans1.guard) != to_string(*trans2.guard))
                            break;

                        if (trans1.inhibited) continue; // TODO Can be generalized

                        bool ok = true;
                        // TODO Check if t2 is k times t1 once we have variable multisets

                        // Check output arcs
                        for (int i = trans1.output_arcs.size() - 1; i >= 0; i--) {
                            const Arc &arc1 = trans1.output_arcs[i];
                            const Arc &arc2 = trans2.output_arcs[i];

                            if (arc1.place != arc2.place || to_string(*arc1.expr) != to_string(*arc2.expr)) {
                                ok = false;
                                break;
                            }
                        }

                        if (!ok) break;

                        // Check input arcs
                        for (int i = trans1.input_arcs.size() - 1; i >= 0; i--) {
                            const Arc &arc1 = trans1.input_arcs[i];
                            const Arc &arc2 = trans2.input_arcs[i];

                            if (arc1.place != arc2.place || to_string(*arc1.expr) != to_string(*arc2.expr)) {
                                ok = false;
                                break;
                            }
                        }

                        if (!ok) break;

                        // Update
                        _applications++;
                        continueReductions = true;
                        red.skipTransition(t2);
                        // op._post just shrunk, so go one back to not miss any
                        if (t2 == touter) {
                            outer--;
                            inner--;
                        }
                        else if (t2 == tinner) inner--;
                        break; // swap
                    }
                }
            }
        }

        return continueReductions;
    }
}