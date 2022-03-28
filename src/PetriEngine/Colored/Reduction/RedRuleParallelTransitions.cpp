/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"
#include "PetriEngine/Colored/Reduction/RedRuleParallelTransitions.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleParallelTransitions::apply(ColoredReducer &red, const std::vector<bool> &inQuery,
                                           QueryType queryType, bool preserveLoops, bool preserveStutter) {

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

                        uint32_t fail = 0;
                        uint32_t mult = std::numeric_limits<uint32_t>::max();

                        // Check output arcs
                        for (int i = trans1.output_arcs.size() - 1; i >= 0; i--) {
                            const Arc &arc1 = trans1.output_arcs[i];
                            const Arc &arc2 = trans2.output_arcs[i];

                            if (arc1.place != arc2.place) {
                                fail = 2;
                                break;
                            }

                            checkMult(fail, mult, *arc1.expr, *arc2.expr);
                            if (fail > 0) break;
                        }

                        if (fail == 2) break;
                        else if (fail == 1) continue;

                        // Check input arcs
                        for (int i = trans1.input_arcs.size() - 1; i >= 0; i--) {
                            const Arc &arc1 = trans1.input_arcs[i];
                            const Arc &arc2 = trans2.input_arcs[i];
                            if (arc1.place != arc2.place) {
                                fail = 2;
                                break;
                            }

                            checkMult(fail, mult, *arc1.expr, *arc2.expr);
                            if (fail > 0) break;
                        }

                        if (fail == 2) break;
                        else if (fail == 1) continue;

                        // Update
                        _applications++;
                        continueReductions = true;
                        red.skipTransition(t2);
                        red._tflags[touter] = 0;
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

    void RedRuleParallelTransitions::checkMult(uint32_t &fail, uint32_t &mult, const ArcExpression &small, const ArcExpression &big) {
        if (mult == std::numeric_limits<uint32_t>::max()) {
            if (to_string(small) == to_string(big)) {
                mult = 1;
                return;
            }

            if (auto sms = ArcVarMultisetVisitor::extract(small)) {
                if (auto bms = ArcVarMultisetVisitor::extract(big)) {
                    if (sms->divides(*bms)) {
                        mult = sms->numberOfTimesThisFitsInto(*bms);
                        return;
                    }
                }
            }

            fail = 1;
            return;
        }

        if (mult == 1 && to_string(small) == to_string(big))
            return;

        if (auto ms1 = ArcVarMultisetVisitor::extract(small)) {
            if (auto ms2 = ArcVarMultisetVisitor::extract(big)) {
                if (*ms1 * mult == ms2) {
                    return;
                }
            }
        }

        fail = 2;
    }
}