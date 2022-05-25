/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/ArcVarMultisetVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRuleRelevance.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleRelevance::isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const {
        return queryType == QueryType::Reach && !preserveStutter && !preserveLoops;
    }

    bool RedRuleRelevance::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                 QueryType queryType, bool preserveLoops, bool preserveStutter) {

        bool changed = false;
        std::vector<uint32_t> wtrans;
        red._tflags.resize(red.transitions().size(), 0);
        std::fill(red._tflags.begin(), red._tflags.end(), 0);
        red._pflags.resize(red.places().size(), 0);
        std::fill(red._pflags.begin(), red._pflags.end(), 0);

        for (uint32_t pid = 0; pid < red.placeCount(); pid++) {
            if (red.hasTimedOut())
                return false;

            if (inQuery.isPlaceUsed(pid)){
                red._pflags[pid] = true;
                const Place &place = red.places()[pid];
                for (auto t : place._post) {
                    if (!red._tflags[t]) {
                        wtrans.push_back(t);
                        red._tflags[t] = true;
                    }
                }
                for (auto t : place._pre) {
                    if (!red._tflags[t]) {
                        wtrans.push_back(t);
                        red._tflags[t] = true;
                    }
                }
            }
        }

        for (uint32_t tid = 0; tid < red.transitionCount(); tid++) {
            if (!red._tflags[tid] && inQuery.isTransitionUsed(tid)) {
                wtrans.push_back(tid);
                red._tflags[tid] = true;
                // All places that a query transition consumes from should be treated as being in the query too.
                for (const auto& arc : red.transitions()[tid].input_arcs){
                    red._pflags[arc.place] = true;
                    const Place &place = red.places()[arc.place];
                    for (auto t : place._post) {
                        if (!red._tflags[t]) {
                            wtrans.push_back(t);
                            red._tflags[t] = true;
                        }
                    }
                    for (auto t : place._pre) {
                        if (!red._tflags[t]) {
                            wtrans.push_back(t);
                            red._tflags[t] = true;
                        }
                    }
                }
            }
        }

        while (!wtrans.empty()) {
            if (red.hasTimedOut()) return false;
            auto t = wtrans.back();
            wtrans.pop_back();
            const Transition &relevantTrans = red.transitions()[t];
            for (const Arc &arc: relevantTrans.input_arcs) {
                const Place &place = red.places()[arc.place];
                red._pflags[arc.place] = true;
                for (uint32_t prtID : place._pre) {
                    if (!red._tflags[prtID]) {
                        const PetriEngine::Colored::Transition& potentiallyRelevantTrans = red.transitions()[prtID];
                        // Loops that do not alter the marking in the place are not considered relevant to the place.
                        const auto& prtIn = red.getInArc(arc.place, potentiallyRelevantTrans);
                        if (prtIn != potentiallyRelevantTrans.input_arcs.end()) {
                            const auto& prtOut = red.getOutArc(potentiallyRelevantTrans, arc.place);
                            if (const auto ms1 = PetriEngine::Colored::extractVarMultiset(*prtIn->expr)){
                                if (const auto ms2 = PetriEngine::Colored::extractVarMultiset(*prtIn->expr)) {
                                    if (ms1 == ms2){
                                        continue;
                                    }
                                }
                            }
                        }
                        red._tflags[prtID] = true;
                        wtrans.push_back(prtID);
                    }
                }
            }
            for (const Arc& inhibitor : red.inhibitorArcs()) {
                if (inhibitor.transition != t)
                    continue;

                red._pflags[inhibitor.place] = true;
                for (const auto prtID : red.places()[inhibitor.place]._post) {
                    if (!red._tflags[prtID]) {
                        // Summary of block: the potentially relevant transition is seen unless it:
                        // - Forms a decreasing loop on 'place' that cannot lower the token count of 'place' below the weight of 'arc'
                        // - Forms a non-decreasing loop on 'place'
                        const Transition &potentiallyRelevantTrans = red.transitions()[prtID];
                        auto prtOut = red.getOutArc(potentiallyRelevantTrans, inhibitor.place);
                        if (prtOut != potentiallyRelevantTrans.output_arcs.end()) {
                            const auto prtIn = red.getInArc(inhibitor.place, potentiallyRelevantTrans);
                            if (prtOut->expr->weight() >= inhibitor.inhib_weight ||
                                prtOut->expr->weight() >= prtIn->expr->weight())
                                continue;
                        }
                        red._tflags[prtID] = true;
                        wtrans.push_back(prtID);
                    }
                }
            }
        }

        for (uint32_t i = red.placeCount(); i > 0;) {
            i--;
            if (red._pflags[i] || red.places()[i].skipped) continue;
            red.skipPlace(i);
            changed = true;
        }

        for (uint32_t i = red.transitionCount(); i > 0;) {
            i--;
            if (red._tflags[i] || red.transitions()[i].skipped) continue;
            red.skipTransition(i);
            changed = true;
        }

        // There must be at least one place.
        if (red.unskippedPlacesCount() == 0) {
            red.addDummyPlace();
        }

        if (changed) _applications++;
        return changed;
    }
}