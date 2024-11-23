#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>

namespace PetriEngine{
    namespace ExplicitColored{
        enum class CheckingBool {
            FALSE,
            TRUE,
            NEVERTRUE
        };
        struct TransitionVariables{
            std::map<Variable_t, std::vector<uint32_t>> possibleValues = std::map<Variable_t, std::vector<uint32_t>>{};
            uint32_t totalBindings = 0;
        };

        class ColoredSuccessorGenerator {
        public:
            explicit ColoredSuccessorGenerator(const ColoredPetriNet& net);
            ~ColoredSuccessorGenerator();

            ColoredPetriNetState next(ColoredPetriNetState& state){
                return _next(state);
            }

            const ColoredPetriNet& net() const {
                return _net;
            }

            Binding getBinding(Transition_t tid, uint32_t bid);
            bool check(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding);
            bool checkInhibitor(const ColoredPetriNetMarking& state, Transition_t tid);
            bool checkPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding);
            CheckingBool firstCheckPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding);
            void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
            void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);
        protected:
            const ColoredPetriNet& _net;
            void _fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& b);

            ColoredPetriNetState _next(ColoredPetriNetState& state){
                auto newState = ColoredPetriNetState{state};
                for (auto tid = state.lastTrans; tid <_net._transitions.size(); tid++){
                    auto bid = tid == state.lastTrans ? state.lastBinding : 0;
                    auto totalBindings = _net._transitions[tid].validVariables.second;
                    if (totalBindings == 0){
                        auto binding = Binding{};
                        if (check(state.marking, tid, binding)){
                            _fire(newState.marking, tid, binding);
                            newState.lastBinding = 0;
                            newState.lastTrans = 0;
                            state.lastBinding = 0;
                            state.lastTrans = tid + 1;
                            return newState;
                        }
                    }else{
                        //Checking whether its impossible before iterating through bindings
                        if (!checkInhibitor(state.marking, tid)){
                            continue;
                        }
                        if (bid == 0){
                            bid++;
                            auto binding = getBinding(tid, bid);
                            auto check = firstCheckPresetAndGuard(state.marking, tid, binding);
                            if (check == CheckingBool::TRUE) {
                                _fire(newState.marking, tid, binding);
                                newState.lastBinding = 0;
                                newState.lastTrans = 0;
                                state.lastBinding = bid;
                                state.lastTrans = tid;
                                return newState;
                            }else if (check == CheckingBool::NEVERTRUE){
                                continue;
                            }
                        }
                        while (++bid <= totalBindings) {
                            auto binding = getBinding(tid, bid);
                            if (checkPresetAndGuard(state.marking, tid, binding)) {
                                _fire(newState.marking, tid, binding);
                                newState.lastBinding = 0;
                                newState.lastTrans = 0;
                                state.lastBinding = bid;
                                state.lastTrans = tid;
                                return newState;
                            }
                        }
                    }
                }
                newState.lastTrans = std::numeric_limits<uint32_t>::max();
                return newState;
            }
        };
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_H */