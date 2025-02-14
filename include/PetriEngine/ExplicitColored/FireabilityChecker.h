//
// Created by joms on 12/12/24.
//

#ifndef FIREABILITYCHECKER_H
#define FIREABILITYCHECKER_H

#include <cstdint>
#include "ColoredPetriNetMarking.h"
#include "ColoredPetriNet.h"
#include "Binding.h"

namespace PetriEngine::ExplicitColored {
    class FireabilityChecker {
    public:
        static bool CanFire(const ColoredPetriNet& cpn, const Transition_t tid, const ColoredPetriNetMarking& state) {
            for (size_t i = cpn._transitionInhibitors[tid]; i < cpn._transitionInhibitors[tid + 1]; i++){
                auto& inhib = cpn._inhibitorArcs[i];
                if (inhib.weight <= state.markings[inhib.from].totalCount()) {
                    return false;
                }
            }

            Binding binding;
            auto totalBindings = cpn._transitions[tid].validVariables.second;

            for (auto i = cpn._transitionArcs[tid].first; i < cpn._transitionArcs[tid].second; i++) {
                auto& arc = cpn._arcs[i];
                if (state.markings[arc.from].totalCount() < arc.expression->getMinimalMarkingCount()) {
                    return false;
                }
            }

            if (totalBindings == 0) {
                return checkWithBinding(binding, cpn, state, tid);
            }

            for (uint32_t bid = 0; bid < totalBindings; ++bid) {
                updateBinding(binding, cpn, tid, bid);
                if (checkWithBinding(binding, cpn, state, tid)) {
                    return true;
                }
            }
            return false;
        }
    private:
        static void updateBinding(Binding& binding, const ColoredPetriNet& cpn, const Transition_t tid, const uint32_t bid) {
            const auto possibleValues = cpn._transitions[tid].validVariables.second;
            if (possibleValues != 0){
                const auto& variables = cpn._transitions[tid].validVariables.first;
                auto interval = possibleValues;
                for (const auto& pair : variables){
                    auto size = pair.second.size();
                    interval /= size;
                    binding.setValue(pair.first, pair.second.at((bid / interval) % size));
                }
            }
        }

        static bool checkWithBinding(const Binding& binding, const ColoredPetriNet& cpn, const ColoredPetriNetMarking& state, const Transition_t tid) {
            if (cpn._transitions[tid].guardExpression != nullptr && !cpn._transitions[tid].guardExpression->eval(binding)){
                return false;
            }
            for (auto i = cpn._transitionArcs[tid].first; i < cpn._transitionArcs[tid].second; i++){
                auto& arc = cpn._arcs[i];
                if (!arc.expression->isSubSet(state.markings[arc.from], binding)) {
                    return false;
                }
            }
            return true;
        }
    };
}


#endif //FIREABILITYCHECKER_H
