//
// Created by joms on 12/12/24.
//

#ifndef FIREABILITYCHECKER_H
#define FIREABILITYCHECKER_H

#include "ColoredSuccessorGenerator.h"
#include "ColoredPetriNetMarking.h"
#include "ColoredPetriNet.h"
#include "Binding.h"

namespace PetriEngine::ExplicitColored {
    class FireabilityChecker {
    public:
        static bool canFire(const ColoredSuccessorGenerator& successorGenerator, const Transition_t tid, const ColoredPetriNetMarking& state) {
            Binding binding;
            const auto totalBindings = successorGenerator.net()._transitions[tid].validVariables.second;
            return successorGenerator.findNextValidBinding(state, tid, 0, totalBindings, binding) != std::numeric_limits<Binding_t>::max();
        }

        //This is no good, but we do not really have deadlock queries
        static bool hasDeadlock (const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking& state) {
            const auto transitionCount = successorGenerator.net().getTransitionCount();
            for (Transition_t tid = 0; tid < transitionCount; tid++) {
                if (canFire(successorGenerator, tid, state)) {
                    return false;
                }
            }
            return true;
        }
    };
}

#endif //FIREABILITYCHECKER_H
