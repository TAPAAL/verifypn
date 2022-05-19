/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */
#include <PetriEngine/Colored/PartitionBuilder.h>
#include <PetriEngine/Colored/EvaluationVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRulePreemptiveFiring.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRulePreemptiveFiring::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                        QueryType queryType,
                                        bool preserveLoops, bool preserveStutter) {
        Colored::PartitionBuilder partition(red.transitions(), red.places());
        bool continueReductions = false;
        const size_t numberofplaces = red.placeCount();
        for (uint32_t p = 0; p < numberofplaces; ++p) {
            if (red.hasTimedOut()) return false;

            auto &place = const_cast<Place &>(red.places()[p]);
            if (place.skipped) continue;
            if (place.marking.empty()) continue;
            if (place.inhibitor) continue;
            if (inQuery.isPlaceUsed(p)) continue;

            // Must be exactly one post, in order to not remove branching
            if (place._post.size() != 1) {
                continue;
            }

            if (!t_is_viable(red, inQuery, place._post[0], p)) continue;

            const Transition &transition = red.transitions()[place._post[0]];

            for (auto &out: transition.output_arcs) {
                auto &otherplace = const_cast<Place &>(red.places()[out.place]);
                otherplace.marking += place.marking;
            }
            place.marking *= 0;


            _applications++;
            continueReductions = true;

        }
        return continueReductions;
    }

    // Search function to see if a transition t can somehow get tokens to place p. Very overestimation, just looking at arcs
    bool RedRulePreemptiveFiring::transition_can_produce_to_place(unsigned int t, uint32_t p, ColoredReducer &red,
                                                                  std::set<uint32_t> &already_checked) const {
        const Transition &transition = red.transitions()[t];
        for (auto &out: transition.output_arcs) {

            //base case
            if (out.place == p) {
                return true;
            }

            if (already_checked.find(out.place) != already_checked.end()) {
                continue;
            } else {
                already_checked.insert(out.place);
            }

            const Place &place = red.places()[out.place];
            if (place.skipped) continue;

            // recursive case
            for (auto &inout: place._post) {
                bool can_produce = (transition_can_produce_to_place(inout, p, red, already_checked));
                if (can_produce) return true;
            }
        }

        return false;
    }

    bool RedRulePreemptiveFiring::t_is_viable(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                              uint32_t t, uint32_t p) {
        //fireability consistency check
        if (inQuery.isTransitionUsed(t)) return false;

        const Transition &transition = red.transitions()[t];
        // Easiest to not handle guards
        if (transition.guard) return false;

        // Check if the transition is currently inhibited
        for (auto &inhibArc: red.inhibitorArcs()) {
            if (inhibArc.place == p && inhibArc.transition == t) {
                auto &place = red.places()[p];
                if (inhibArc.inhib_weight <= place.marking.size()) {
                    return false;
                }
            }
        }

        //could also relax this, but seems difficult
        if (transition.input_arcs.size() > 1) return false;

        //Could relax this, and only move some tokens, or check distinct size on marking
        auto &place = red.places()[p];
        const auto &in = red.getInArc(p, transition);
        if ((place.marking.size() % in->expr->weight()) != 0) {
            return false;
        }

        // - postset cannot inhibit or be in query
        for (auto &out: transition.output_arcs) {
            auto &outPlace = red.places()[out.place];
            if (inQuery.isPlaceUsed(out.place) || outPlace.inhibitor) {
                return false;
            }


            //for fireability consistency. We don't want to put tokens to a place enabling transition
            for (auto &tin: outPlace._post) {
                if (inQuery.isTransitionUsed(tin)) {
                    return false;
                }
            }

            //todo could relax this, and instead of simply copying the tokens to the new place, then update them according to the out arc expression
            //todo or simple extension, check if constant color on the out arc
            if (to_string(*out.expr) != to_string(*in->expr)) {
                return false;
            }
        }

        // - Make sure that we do not produce tokens to something that can produce tokens to our preset. To disallow infinite use of this rule by looping
        if (place._pre.size() > 0) {
            if (in->expr->weight() != 1) return false;
            std::set<uint32_t> already_checked;
            if (transition_can_produce_to_place(t, p, red, already_checked)) return false;
        }

        return true;
    }
}