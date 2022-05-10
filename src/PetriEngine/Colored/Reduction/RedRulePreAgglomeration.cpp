/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/Reduction/RedRulePreAgglomeration.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRulePreAgglomeration::isApplicable(QueryType queryType, bool preserveLoops, bool preserveStutter) const {
        return queryType != CTL && !preserveStutter;
    }

    bool RedRulePreAgglomeration::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                        QueryType queryType, bool preserveLoops, bool preserveStutter) {
        bool continueReductions = false;
        bool changed = true;
        bool atomic_viable = (queryType == Reach) && !preserveLoops;

        // Apply repeatedly
        while (changed) {
            changed = false;

            for (uint32_t pid = 0; pid < red.placeCount(); pid++) {
                if (red.hasTimedOut())
                    return false;
                // Limit explosion
                if (red.origTransitionCount() * 2 < red.unskippedTransitionsCount())
                    return false;

                const Place &place = red.places()[pid];

                // Limit large applications
                if (place._pre.size() > explosion_limiter){
                    continue;
                }

                // T/S8--1, T/S1, T/S3
                if (place.skipped || place.inhibitor || inQuery.isPlaceUsed(pid) || !place.marking.empty() || place._pre.empty() ||
                    place._post.empty())
                    continue;


                const auto presize = place._pre.size();
                const auto postsize = place._post.size();
                bool ok = true;
                uint32_t i = 0, j = 0;

                // Check that producers and consumers are disjoint, and not in a fireability query
                // T/S4, T/S2
                while (i < presize && j < postsize) {
                    if (place._pre[i] < place._post[j]) {
                        if (inQuery.isTransitionUsed(place._pre[i])) {
                            ok = false;
                            break;
                        }
                        i++;
                    }
                    else if (place._post[j] < place._pre[i]) {
                        if (inQuery.isTransitionUsed(place._post[j])) {
                            ok = false;
                            break;
                        }
                        j++;
                    }
                    else {
                        ok = false;
                        break;
                    }
                }
                if (!ok) continue;

                for ( ; i < presize; i++) {
                    if (inQuery.isTransitionUsed(place._pre[i])) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) continue;

                for ( ; j < postsize; j++) {
                    if (inQuery.isTransitionUsed(place._post[j])) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) continue;

                std::vector<bool> todo (postsize, true);
                bool todoAllGood = true;
                // S11, S12
                std::vector<bool> kIsAlwaysOne (postsize, true);

                for (const auto& prod : place._pre){
                    const Transition& producer = red.transitions()[prod];
                    // X8.1, X6
                    if(producer.inhibited || producer.output_arcs.size() != 1  || producer.guard != nullptr){
                        ok = false;
                        break;
                    }

                    const CArcIter& prodArc = red.getOutArc(producer, pid);
                    uint32_t kw;

                    // T9, S6
                    if(prodArc->expr->is_single_color()){
                        kw = prodArc->expr->weight();
                    } else {
                        ok = false;
                        break;
                    }

                    for (uint32_t n = 0; n < place._post.size(); n++) {
                        const PetriEngine::Colored::Transition& consumer = red.transitions()[place._post[n]];
                        const CArcIter& consArc = red.getInArc(pid, consumer);
                        uint32_t w = consArc->expr->weight();
                        // (T9, S6), S10, T10
                        if (atomic_viable){
                            if (!consArc->expr->is_single_color() || kw % w != 0) {
                                todo[n] = false;
                                todoAllGood = false;
                            } else if (kw != w) {
                                kIsAlwaysOne[n] = false;
                            }
                        } else if (!consArc->expr->is_single_color() || kw != w) {
                            ok = false;
                            break;
                        }
                    }

                    // Check if we have any qualifying consumers left
                    if (!ok || (!todoAllGood && std::lower_bound(todo.begin(), todo.end(), true) == todo.end())){
                        ok = false;
                        break;
                    }

                    for (const auto& prearc : producer.input_arcs){
                        const Place& preplace = red.places()[prearc.place];
                        // T/S8--3, T/S7--2
                        if (preplace.inhibitor || inQuery.isPlaceUsed(prearc.place)){
                            ok = false;
                            break;
                        } else if (!atomic_viable) {
                            // For reachability, we can do free agglomeration which avoids this condition
                            // T5
                            for(uint32_t alternative : preplace._post){
                                // T5; Transitions in place.pre are exempt from this check
                                if (std::lower_bound(place._pre.begin(), place._pre.end(), alternative) != place._pre.end())
                                    continue;

                                const Transition& alternativeConsumer = red.transitions()[alternative];
                                // T5; Transitions outside place.pre are not allowed to alter the contents of preplace
                                if (red.getInArc(prearc.place, alternativeConsumer)->expr == red.getOutArc(alternativeConsumer, prearc.place)->expr){
                                    ok = false;
                                    break;
                                }
                            }
                        }
                        if (!ok) break;
                    }

                    if (!ok) break;
                }

                if (!ok) continue;

                std::vector<uint32_t> originalConsumers = place._post;
                std::vector<uint32_t> originalProducers = place._pre;
                for (uint32_t n = 0; n < originalConsumers.size(); n++)
                {
                    if (red.hasTimedOut())
                        return false;
                    if (!todo[n])
                        continue;
                    ok = true;

                    const Transition &consumer = red.transitions()[originalConsumers[n]];
                    if (consumer.guard != nullptr) continue;

                    // T trivially passes these because of T5 earlier
                    if (atomic_viable){
                        // S12
                        if (!kIsAlwaysOne[n] && consumer.input_arcs.size() != 1) {
                            continue;
                        }
                        // S11
                        if (!kIsAlwaysOne[n]) {
                            for (const auto& conspost : consumer.output_arcs) {
                                if (red.places()[conspost.place].inhibitor || (queryType != Reach && inQuery.isPlaceUsed(conspost.place))) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (!ok) continue;

                    const auto& consArc = red.getInArc(pid, consumer);
                    uint32_t w = consArc->expr->weight();
                    
                    // Update
                    for (const auto& prod : originalProducers){
                        const Transition& producer = red.transitions()[prod];
                        const CArcIter proArc = red.getOutArc(producer, pid);

                        uint32_t k = 1;
                        if (!kIsAlwaysOne[n]){
                            k = proArc->expr->weight() / w;
                        }
                        
                        // One for each number of firings of consumer possible after one firing of producer
                        for (uint32_t k_i = 1; k_i <= k; k_i++){
                            // Create new transition with effect of firing the producer, and then the consumer k_i times
                            auto tid = red.newTransition(nullptr);

                            // Re-fetch the transition references as they might be invalidated?
                            const Transition &producerPrime = red.transitions()[prod];
                            const Transition &consumerPrime = red.transitions()[originalConsumers[n]];

                            // Arcs from consumer
                            for (const auto& arc : consumerPrime.output_arcs) {
                                ArcExpression_ptr expr = arc.expr;
                                if (k_i > 1){
                                    red.addOutputArc(tid, arc.place, std::make_shared<PetriEngine::Colored::ScalarProductExpression>(std::shared_ptr(expr), k_i));
                                } else {
                                    red.addOutputArc(tid, arc.place, expr);
                                }
                            }
                            for (const auto& arc : consumerPrime.input_arcs){
                                if (arc.place != pid){
                                    ArcExpression_ptr expr = arc.expr;
                                    red.addInputArc(arc.place, tid, expr, arc.inhib_weight);
                                }
                            }

                            for (const auto& arc : producerPrime.input_arcs){
                                ArcExpression_ptr expr = arc.expr;
                                red.addInputArc(arc.place, tid, expr, arc.inhib_weight);
                            }

                            if (k_i != k){
                                red.addOutputArc(tid, pid, std::make_shared<PetriEngine::Colored::ScalarProductExpression>(std::shared_ptr(proArc->expr), k-k_i));
                            }
                        }
                    }
                    red.skipTransition(originalConsumers[n]);
                    changed = true;
                    _applications++;
                }

                if (place._post.empty()) {
                    auto transitions = place._pre;
                    for (uint32_t tran_id : transitions)
                        red.skipTransition(tran_id);
                    red.skipPlace(pid);
                }

                red.consistent();
            }

            continueReductions |= changed;
        }

        red.consistent();
        return continueReductions;
    }
}
