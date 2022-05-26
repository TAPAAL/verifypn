/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include <PetriEngine/Colored/VarReplaceVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRulePreAgglomeration.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"
#include "PetriEngine/Colored/VariableVisitor.h"
#include "PetriEngine/Colored/IsVariableVisitor.h"

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
                std::set<const Variable*> prodArcVars;
                std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> allProdTuples;

                for (const auto& prod : place._pre){
                    const Transition& producer = red.transitions()[prod];

                    // X8.1, X6
                    if(producer.inhibited || producer.output_arcs.size() != 1){
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

                    uint32_t prodTuplesStatus = allProdTuples.size();
                    Colored::VariableVisitor::get_variables(*prodArc->expr, prodArcVars, allProdTuples);
                    // if there are any tuples, there has to be exactly 1 on all the arcs.
                    if (!allProdTuples.empty() && allProdTuples.size() != prodTuplesStatus + 1) {
                        ok = false;
                        break;
                    }

                    for (uint32_t n = 0; n < place._post.size(); n++) {
                        const PetriEngine::Colored::Transition& consumer = red.transitions()[place._post[n]];
                        const CArcIter& consArc = red.getInArc(pid, consumer);
                        uint32_t w = consArc->expr->weight();

                        // (T9, S6), S10, T10, T12
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
                            if (preplace._post.size() > 1){
                                ok = false;
                                break;
                            }
                        }
                        if (!ok) break;
                    }

                    if (!ok) break;
                }

                if (!ok) continue;

                // Every tuple has to line up with each other, so we save one to compare them all to.
                std::vector<const Colored::ColorExpression*>* referenceTuple = nullptr;
                if (!allProdTuples.empty()){
                    for (auto& tuple : allProdTuples){
                        if (!referenceTuple){
                            // We only support tuples of variables
                            IsVariableVisitor v;
                            for (auto& element : tuple.second){
                                if(!v.isVariableExpr(element)){
                                    ok = false;
                                    break;
                                }
                            }
                            if (!ok) break;
                            referenceTuple = &tuple.second;
                            continue;
                        } else if (tuple.second != *referenceTuple){
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok) continue;

                std::vector<uint32_t> originalConsumers = place._post;
                std::vector<uint32_t> originalProducers = place._pre;
                // a pair containing prodHangingGuardVarRisk, prodHangingArcVar
                std::pair<bool, bool> prodHangingVars = _prodHangingGuardVar(red, pid, originalProducers);

                for (uint32_t n = 0; n < originalConsumers.size(); n++) {
                    if (red.hasTimedOut())
                        return false;
                    if (!todo[n])
                        continue;
                    ok = true;

                    const Transition &consumer = red.transitions()[originalConsumers[n]];

                    if (atomic_viable) {
                        // S12
                        if (!kIsAlwaysOne[n] && consumer.input_arcs.size() != 1) {
                            continue;
                        }
                        // S11
                        if (!kIsAlwaysOne[n]) {
                            for (const auto &conspost: consumer.output_arcs) {
                                if (red.places()[conspost.place].inhibitor) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                        if (!ok) continue;
                    }

                    const auto &consArc = red.getInArc(pid, consumer);
                    uint32_t w = consArc->expr->weight();

                    // Identify the variables of the consumer
                    bool consHangingGuardVarRisk = false;
                    std::set<const Variable *> consVars;
                    std::set<const Variable *> consArcVars;
                    std::set<const Variable *> consGuardVars;
                    std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> consTuples;

                    Colored::VariableVisitor::get_variables(*consArc->expr, consArcVars, consTuples);

                    if (prodHangingVars.second) {
                        // The hanging variable checks need the arc variables and guard variables in separate sets
                        if (consumer.guard)
                            Colored::VariableVisitor::get_variables(*consumer.guard, consGuardVars);

                        for (auto &var: consArcVars) {
                            if (prodHangingVars.second && consGuardVars.find(var) != consGuardVars.end()) {
                                // prodHangingArcVar && consHangingGuardVarRisk is NG
                                consHangingGuardVarRisk = true;
                                break;
                            }
                        }
                    }

                    // The hanging guards that could not be caught by the producer's arcs have to be caught by the consumer's arcs now, or the agglomeration cant go on.
                    if (prodHangingVars.first || consHangingGuardVarRisk) {
                        for (auto &arc: consumer.input_arcs) {
                            if (arc.place != consArc->place) {
                                Colored::VariableVisitor::get_variables(*arc.expr, consVars);
                            }
                        }
                        for (auto &arc: consumer.output_arcs) {
                            if (arc.place != consArc->place) {
                                Colored::VariableVisitor::get_variables(*arc.expr, consVars);
                            }
                        }

                        for (auto &var: consArcVars) {
                            if (consVars.find(var) == consVars.end()) {
                                // If the producer has a hanging guard variable, we cannot allow consArc to also have hanging variables
                                ok = false;
                                break;
                            }
                        }
                    }

                    if (!ok) {
                        if (atomic_viable) {
                            todo[n] = false;
                            todoAllGood = false;
                            continue;
                        } else {
                            ok = false;
                            break;
                        }
                    }

                    // referenceTuple being nullptr here means there were no tuples in the producers
                    assert(consTuples.empty() == (referenceTuple == nullptr));
                    if (!consTuples.empty()){
                        IsVariableVisitor varvis;
                        for (auto& tuple : consTuples){
                            if (tuple.second.size() == referenceTuple->size()){
                                std::unordered_map<std::string , uint32_t> tuplematching;
                                for (uint32_t ii = 0; ii < consTuples.size(); ii++){
                                    if (!varvis.isVariableExpr(tuple.second[ii])){
                                        // Check that the tuple is all variables, now for the consumer
                                        ok = false;
                                        break;
                                    } else if (tuplematching[varvis.getVariableName(tuple.second[ii])] != tuplematching[varvis.getVariableName((*referenceTuple)[ii])]){
                                        // This branch is reached if one of the transitions is a duplicate, while the other is not, or is a duplicate too but of a different variable
                                        ok = false;
                                        break;
                                    } else {
                                        tuplematching[varvis.getVariableName(tuple.second[ii])] = ii;
                                        tuplematching[varvis.getVariableName((*referenceTuple)[ii])] = ii;
                                    }
                                }
                            } else {
                                ok = false;
                            }
                            if (!ok) break;
                        }
                    }

                    if (!ok){
                        if (atomic_viable) {
                            todo[n] = false;
                            todoAllGood = false;
                            continue;
                        } else {
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok) continue;

                // Update
                for (uint32_t n = 0; n < originalConsumers.size(); n++)
                {
                    if (!todo[n])
                        continue;
                    red.renameVariables(originalConsumers[n]);
                    const Transition &consumer = red.transitions()[originalConsumers[n]];
                    const auto &consArc = red.getInArc(pid, consumer);

                    uint32_t w = consArc->expr->weight();
                    std::set<const Variable *> consVars;
                    std::set<const Variable *> consArcVars;
                    std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> consTuples;
                    Colored::VariableVisitor::get_variables(*consArc->expr, consArcVars, consTuples);

                    if(consumer.guard){
                        Colored::VariableVisitor::get_variables(*consumer.guard, consVars);
                    }
                    for (auto& arc : consumer.input_arcs){
                        Colored::VariableVisitor::get_variables(*arc.expr, consVars);
                    }
                    for (auto& arc : consumer.output_arcs){
                        Colored::VariableVisitor::get_variables(*arc.expr, consVars);
                    }

                    for (const auto& prod : originalProducers){
                        const Transition& producer = red.transitions()[prod];
                        const Transition& consumer2 = red.transitions()[originalConsumers[n]];
                        const CArcIter proArc = red.getOutArc(producer, pid);

                        std::set<const Variable*> pairVars;
                        std::unordered_map<uint32_t, std::vector<const Colored::ColorExpression*>> prodTuples;
                        Colored::VariableVisitor::get_variables(*proArc->expr, pairVars, prodTuples);
                        IsVariableVisitor varvis;

                        std::unordered_map<std::string, const Variable*> varReplacementMap;
                        if (!prodTuples.empty()){
                            for (uint32_t tupleIndex = 0; tupleIndex < referenceTuple->size(); tupleIndex++){
                                const Variable* prodVar = varvis.getVariable(prodTuples.at(1)[tupleIndex]);
                                const Variable* consVar = varvis.getVariable(consTuples.at(1)[tupleIndex]);
                                if (varReplacementMap[prodVar->name] == nullptr && varReplacementMap[consVar->name] == nullptr){
                                    auto* newVar = new Variable{*producer.name + *consumer2.name + prodVar->name + consVar->name, prodVar->colorType};
                                    red.addVariable(newVar);
                                    varReplacementMap[prodVar->name] = newVar;
                                    varReplacementMap[consVar->name] = newVar;
                                } else if (varReplacementMap[prodVar->name] == nullptr){
                                    varReplacementMap[prodVar->name] = varReplacementMap[consVar->name];
                                } else if (varReplacementMap[consVar->name] == nullptr){
                                    varReplacementMap[consVar->name] = varReplacementMap[prodVar->name];
                                } else {
                                    // Two variables that were thought to be separate actually need to be the same, fix it
                                    // Only applies in cases such as --(k,k,i)->(place)--(k,i,i)->
                                    const Variable* emergencyVar1 = varReplacementMap[prodVar->name];
                                    const Variable* emergencyVar2 = varReplacementMap[consVar->name];
                                    for (auto& pair : varReplacementMap){
                                        if (pair.second == emergencyVar1){
                                            pair.second = emergencyVar2;
                                        }
                                    }
                                }
                            }
                        } else {
                            for (auto& pvar : pairVars){
                                if (varReplacementMap[pvar->name] == nullptr){
                                    auto newVar = new Variable{*producer.name + *consumer2.name + pvar->name, pvar->colorType};
                                    red.addVariable(newVar);
                                    varReplacementMap[pvar->name] = newVar;
                                    for (auto& cvar : consArcVars){
                                        varReplacementMap[cvar->name] = newVar;
                                    }
                                }
                            }
                        }

                        pairVars.insert(consVars.begin(), consVars.end());

                        if(producer.guard){
                            Colored::VariableVisitor::get_variables(*producer.guard, pairVars);
                        }
                        for (auto& arc : producer.input_arcs){
                            Colored::VariableVisitor::get_variables(*arc.expr, pairVars);
                        }
                        for (auto& arc : producer.output_arcs){
                            Colored::VariableVisitor::get_variables(*arc.expr, pairVars);
                        }

                        for (auto& var : pairVars){
                            if (varReplacementMap[var->name] == nullptr){
                                auto* newVar = new Variable{*producer.name + *consumer2.name + var->name, var->colorType};
                                red.addVariable(newVar);
                                varReplacementMap[var->name] = newVar;
                            }
                        }

                        VarReplaceVisitor varReplacevis = VarReplaceVisitor(varReplacementMap);

                        uint32_t k = 1;
                        if (!kIsAlwaysOne[n]){
                            k = proArc->expr->weight() / w;
                        }
                        GuardExpression_ptr mergedguard = nullptr;
                        if (consumer2.guard != nullptr && producer.guard != nullptr){
                            mergedguard = std::make_shared<PetriEngine::Colored::AndExpression>(varReplacevis.makeReplacementGuard(producer.guard), varReplacevis.makeReplacementGuard(consumer2.guard));
                        } else if (consumer2.guard != nullptr){
                            mergedguard = varReplacevis.makeReplacementGuard(consumer2.guard);
                        } else if (producer.guard != nullptr){
                            mergedguard = varReplacevis.makeReplacementGuard(producer.guard);
                        }

                        // One for each number of firings of consumer possible after one firing of producer
                        for (uint32_t k_i = 1; k_i <= k; k_i++){
                            // Create new transition with effect of firing the producer, and then the consumer k_i times
                            auto tid = red.newTransition(mergedguard);

                            // Separate variables for the k_i firing versions.
                            if (k_i > 1){
                                for (auto& kvPair : varReplacementMap) {
                                    auto* newVar = new Variable{kvPair.first + "k" + std::to_string(k_i), kvPair.second->colorType};
                                    red.addVariable(newVar);
                                    kvPair.second = newVar;
                                }
                            }

                            // Re-fetch the transition references as they might be invalidated?
                            const Transition &producerPrime = red.transitions()[prod];
                            const Transition &consumerPrime = red.transitions()[originalConsumers[n]];

                            // Arcs from consumer
                            for (const auto& arc : consumerPrime.output_arcs) {
                                ArcExpression_ptr expr = varReplacevis.makeReplacementArcExpr(arc.expr);
                                if (k_i > 1){
                                    red.addOutputArc(tid, arc.place, std::make_shared<PetriEngine::Colored::ScalarProductExpression>(std::shared_ptr(expr), k_i));
                                } else {
                                    red.addOutputArc(tid, arc.place, expr);
                                }
                            }
                            for (const auto& arc : consumerPrime.input_arcs){
                                if (arc.place != pid){
                                    ArcExpression_ptr expr = varReplacevis.makeReplacementArcExpr(arc.expr);
                                    red.addInputArc(arc.place, tid, expr, arc.inhib_weight);
                                }
                            }

                            for (const auto& arc : red.inhibitorArcs()){
                                if (arc.transition == originalConsumers[n]){
                                    ArcExpression_ptr expr = nullptr;
                                    red.addInputArc(arc.place, tid, expr, arc.inhib_weight);
                                }
                            }

                            for (const auto& arc : producerPrime.input_arcs){
                                ArcExpression_ptr expr = varReplacevis.makeReplacementArcExpr(arc.expr);
                                red.addInputArc(arc.place, tid, expr, arc.inhib_weight);
                            }

                            if (k_i != k){
                                red.addOutputArc(tid, pid, std::make_shared<PetriEngine::Colored::ScalarProductExpression>(varReplacevis.makeReplacementArcExpr(proArc->expr), k-k_i));
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

    std::pair<bool, bool> RedRulePreAgglomeration::_prodHangingGuardVar(ColoredReducer& red, uint32_t pid, const std::vector<uint32_t>& originalProducers) {
        bool hangingGuardVar_risk = false;
        bool hangingArcVar = false;
        for (const uint32_t prod : originalProducers){
            const Transition& producer = red.transitions()[prod];
            const CArcIter prodArc = red.getOutArc(producer, pid);
            std::set<const Variable*> prodArcVars;
            std::set<const Variable*> prodGuardVars;
            Colored::VariableVisitor::get_variables(*prodArc->expr, prodArcVars);

            if(producer.guard){
                Colored::VariableVisitor::get_variables(*producer.guard, prodGuardVars);
                for (auto& var : prodArcVars){
                    if (prodGuardVars.find(var) != prodGuardVars.end()){
                        hangingGuardVar_risk = true;
                        break;
                    }
                }
            }

            std::set<const Variable*> prodVars;

            for (auto& arc : producer.input_arcs){
                if (arc.place != prodArc->place){
                    Colored::VariableVisitor::get_variables(*arc.expr, prodVars);
                }
            }
            for (auto& arc : producer.output_arcs){
                if (arc.place != prodArc->place){
                    Colored::VariableVisitor::get_variables(*arc.expr, prodVars);
                }
            }

            for (auto& var : prodArcVars){
                if (prodVars.find(var) == prodVars.end()){
                    // There is indeed a hanging guard variable here;
                    hangingArcVar = true;
                    break;
                }
            }
            if (hangingGuardVar_risk && hangingArcVar) break;
        }
        // hangingGuardVar_risk is only actually a problem if there is a hangingArcVar too.
        return std::pair{(hangingGuardVar_risk && hangingArcVar), hangingArcVar};
    }
}

