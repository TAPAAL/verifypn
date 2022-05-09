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

        // Remove places in parallel where one accumulates tokens while the other disable their post set

        bool continueReductions = false;

        red._pflags.resize(red.placeCount(), 0);
        std::fill(red._pflags.begin(), red._pflags.end(), 0);

        for (uint32_t tid_outer = 0; tid_outer < red.transitionCount(); ++tid_outer) {
            for (size_t aid_outer = 0; aid_outer < red.transitions()[tid_outer].output_arcs.size(); ++aid_outer) {

                auto pid_outer = red.transitions()[tid_outer].output_arcs[aid_outer].place;
                if (red._pflags[pid_outer] > 0) continue;
                red._pflags[pid_outer] = 1;

                if (red.hasTimedOut()) return false;

                const Place &pout = red.places()[pid_outer];
                if (pout.skipped) continue;

                for (size_t aid_inner = aid_outer + 1; aid_inner < red.transitions()[tid_outer].output_arcs.size(); ++aid_inner) {
                    if (pout.skipped) break;
                    auto pid_inner = red.transitions()[tid_outer].output_arcs[aid_inner].place;
                    if (red.places()[pid_inner].skipped) continue;

                    if (red.places()[pid_inner].type != red.places()[pid_outer].type) continue;

                    for (size_t swp = 0; swp < 2; ++swp) {
                        if (red.hasTimedOut()) return false;
                        if (red.places()[pid_inner].skipped ||
                            red.places()[pid_outer].skipped)
                            break;

                        uint p1 = pid_outer;
                        uint p2 = pid_inner;

                        assert(p1 != p2);
                        if (swp == 1) std::swap(p1, p2);

                        if (inQuery.isPlaceUsed(p2)) continue;

                        const Place &place1 = red.places()[p1];
                        const Place &place2 = red.places()[p2];

                        if (place2._pre.empty() || place1._post.empty()) continue;

                        if (place1._post.size() < place2._post.size() ||
                            place1._pre.size() > place2._pre.size())
                            continue;

                        // Initial marking must share support
                        if (place1.marking.distinctSize() != place2.marking.distinctSize()
                                || (place1.marking + place2.marking).distinctSize() != place1.marking.distinctSize())
                            break;

                        bool ok = true;

                        double maxDrainRatio = 0;

                        uint32_t i = 0, j = 0;
                        while (i < place1._post.size() && j < place2._post.size()) {

                            uint32_t p1t = place1._post[i];
                            uint32_t p2t = place2._post[j];

                            if (p2t < p1t) {
                                // place2._post is not a subset of place1._post
                                ok = false;
                                break;
                            }

                            i++;
                            if (p2t > p1t) {
                                swp = 2; // We can't remove p1, so don't try swap
                                continue;
                            }
                            j++;

                            const Transition &tran = red.transitions()[p1t];
                            const auto &p1Arc = red.getInArc(p1, tran);
                            const auto &p2Arc = red.getInArc(p2, tran);

                            if (to_string(*p1Arc->expr) == to_string(*p2Arc->expr)) {
                                maxDrainRatio = std::max(maxDrainRatio, 1.0);
                                continue;
                            }

                            const auto ms1 = PetriEngine::Colored::extractVarMultiset(*p1Arc->expr);
                            const auto ms2 = PetriEngine::Colored::extractVarMultiset(*p2Arc->expr);

                            // ms1 and ms2 must share support

                            if (!ms1 || !ms2 || ms1->distinctSize() != ms2->distinctSize() || ((*ms1) + (*ms2)).distinctSize() != ms1->distinctSize()) {
                                ok = false;
                                swp = 2;
                                break;
                            }

                            for (const auto& [varvec, multiplicity] : *ms1) {
                                maxDrainRatio = std::max(maxDrainRatio, (double)(*ms2)[varvec] / (double)multiplicity);
                            }
                        }

                        if (!ok || j != place2._post.size()) continue;

                        if (!(place1.marking * maxDrainRatio).isSubsetOrEqTo(place2.marking)) continue;

                        i = 0, j = 0;
                        while (i < place1._pre.size() && j < place2._pre.size()) {
                            if (red.hasTimedOut()) return false;

                            uint32_t p1t = place1._pre[i];
                            uint32_t p2t = place2._pre[j];

                            if (p1t < p2t) {
                                // place1._pre is not a subset of place2._pre
                                ok = false;
                                break;
                            }

                            j++;
                            if (p1t > p2t) {
                                swp = 2; // We can't remove p1, so don't try swap
                                continue;
                            }
                            i++;

                            const Transition &tran = red.transitions()[p2t];
                            const auto &p2Arc = red.getOutArc(tran, p2);
                            const auto &p1Arc = red.getOutArc(tran, p1);

                            if (to_string(*p1Arc->expr) == to_string(*p2Arc->expr) && maxDrainRatio > 1.0) {
                                ok = false;
                                break;
                            }

                            const auto ms1 = PetriEngine::Colored::extractVarMultiset(*p1Arc->expr);
                            const auto ms2 = PetriEngine::Colored::extractVarMultiset(*p2Arc->expr);

                            // ms1 and ms2 must share support

                            if (!ms1 || !ms2 || ms1->distinctSize() != ms2->distinctSize() || ((*ms1) + (*ms2)).distinctSize() != ms1->distinctSize()) {
                                ok = false;
                                swp = 2;
                                break;
                            }

                            for (const auto& [varvec, multiplicity] : *ms1) {
                                if (maxDrainRatio > (double)(*ms2)[varvec] / (double)multiplicity) {
                                    ok = false;
                                    break;
                                }
                            }
                            if (!ok) break;
                        }

                        if (!ok || i != place1._pre.size()) continue;

                        continueReductions = true;
                        _applications++;
                        red.skipPlace(p2);

                        // p2 has now been removed from tid_outer.output_arcs, so update arc indexes to not miss any places
                        if (p2 == pid_outer) {
                            aid_outer--;
                            aid_inner--;
                        } else if (p2 == pid_inner) aid_inner--;
                        break;
                    }
                }
            }
        }
        red.consistent();
        return continueReductions;
    }
}