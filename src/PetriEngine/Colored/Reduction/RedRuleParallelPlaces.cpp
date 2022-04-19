/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/Reduction/RedRuleParallelPlaces.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRuleParallelPlaces::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                      QueryType queryType, bool preserveLoops, bool preserveStutter) {

        // Remove places which input and output is k times another place's input and output

        bool continueReductions = false;

        auto &places = red.places();
        auto &transitions = red.transitions();

        red._pflags.resize(places.size());
        std::fill(red._pflags.begin(), red._pflags.end(), 0);

        for (uint32_t touter = 0; touter < transitions.size(); touter++) {
            for (int outer = 0; outer < transitions[touter].output_arcs.size(); outer++) {

                auto pouter = transitions[touter].output_arcs[outer].place;
                if (red._pflags[pouter] > 0) continue;
                red._pflags[pouter] = 1;
                if (red.hasTimedOut()) return false;

                const Place &pout = places[pouter];

                if (pout.skipped) continue;

                for (uint32_t inner = outer + 1; inner < transitions[touter].output_arcs.size(); inner++) {
                    if (pout.skipped) break;

                    auto pinner = transitions[touter].output_arcs[inner].place;
                    if (places[pinner].skipped) continue;

                    if (pouter == pinner) continue;
                    if (places[pinner].type != places[pouter].type) continue;

                    for (size_t swp = 0; swp < 2; swp++) {

                        if (red.hasTimedOut()) return false;
                        if (places[pinner].skipped || places[pouter].skipped) break;

                        uint32_t p1 = pouter;
                        uint32_t p2 = pinner;
                        if (swp == 1) std::swap(p1, p2);

                        // We will now check if p2 can be removed

                        if (inQuery.isPlaceUsed(p2)) continue;

                        const Place &place1 = places[p1];
                        const Place &place2 = places[p2];

                        if (place1._pre.size() > place2._pre.size() ||
                            place1._post.size() < place2._post.size())
                            continue;

                        if (place2.inhibitor) continue; // TODO can be generalized, also with k scaling

                        double mult = 1.0;

                        int fail = 0;
                        size_t j = 0;
                        for (size_t i = 0; i < place2._post.size(); i++) {

                            // place1 may have consumers that place2 does not
                            while (j < place1._post.size() && place1._post[j] < place2._post[i]) j++;
                            if (place1._post.size() <= j || place1._post[j] != place2._post[i]) {
                                fail = 2;
                                break;
                            }

                            const Transition &trans = transitions[place1._post[j]];
                            auto a1 = red.getInArc(p1, trans);
                            auto a2 = red.getInArc(p2, trans);
                            assert(a1 != trans.input_arcs.end());
                            assert(a2 != trans.input_arcs.end());

                            if (to_string(*a1->expr) == to_string(*a2->expr)) {
                                continue; // mult is already at least 1
                            } else if (auto ms1 = PetriEngine::Colored::extractVarMultiset(*a1->expr)) {
                                if (auto ms2 = PetriEngine::Colored::extractVarMultiset(*a2->expr)) {
                                    if (auto k = ms1->scaleRequiredToCover(*ms2)) {
                                        mult = std::max(mult, *k);
                                        continue;
                                    }
                                }
                            }
                            fail = 2;
                            break;
                        }

                        if (fail == 2) break;

                        // TODO Could be more precise with fuzzy multisets
                        if (!(place1.marking * (uint32_t)std::ceil(mult)).isSubsetOrEqTo(place2.marking)) continue;

                        j = 0;
                        for (size_t i = 0; i < place1._pre.size(); i++) {

                            // place2 may have producers that place1 does not
                            while (j < place2._pre.size() && place2._pre[j] < place1._pre[i]) j++;
                            if (j == place2._pre.size() || place1._pre[j] != place2._pre[i]) {
                                fail = 2;
                                break;
                            }

                            const Transition &trans = transitions[place1._pre[j]];
                            auto a1 = red.getOutArc(trans, p1);
                            auto a2 = red.getOutArc(trans, p2);
                            assert(a1 != trans.output_arcs.end());
                            assert(a2 != trans.output_arcs.end());

                            if (mult == 1.0 && to_string(*a1->expr) == to_string(*a2->expr)) {
                                continue;
                            } else if (auto ms1 = PetriEngine::Colored::extractVarMultiset(*a1->expr)) {
                                if (auto ms2 = PetriEngine::Colored::extractVarMultiset(*a2->expr)) {
                                    // TODO Could be more precise with fuzzy multisets
                                    if ((*ms1 * (uint32_t)std::ceil(mult)).isSubsetOrEqTo(*ms2)) {
                                        continue;
                                    }
                                }
                            }
                            fail = 1;
                            break;
                        }

                        if (fail == 2) break;
                        else if (fail == 1) continue;

                        // Remove p2
                        _applications++;
                        continueReductions = true;
                        red.skipPlace(p2);
                        red._pflags[pouter] = 0;
                        // touter.output_arcs have shrunk, so go one back to not miss any
                        if (p2 == pouter) {
                            outer--;
                            inner--;
                        }
                        else if (p2 == pinner) inner--;
                        break;
                    }
                }
            }
        }

        return continueReductions;
    }
}