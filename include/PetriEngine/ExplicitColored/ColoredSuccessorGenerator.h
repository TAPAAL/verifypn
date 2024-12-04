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

            ColoredPetriNetState next(ColoredPetriNetState& state) const {
                if (state.forRDFS) {
                    return _nextOneTrans(state);
                }
                return _next(state);
            }

            const ColoredPetriNet& net() const {
                return _net;
            }

            Binding getBinding(Transition_t tid, uint32_t bid) const;
            bool check(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
            bool checkInhibitor(const ColoredPetriNetMarking& state, Transition_t tid) const;
            bool checkPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
            CheckingBool firstCheckPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
            void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
            void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        protected:
            const ColoredPetriNet& _net;
            void _fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;

            ColoredPetriNetState _next(ColoredPetriNetState &state) const {
                auto newState = ColoredPetriNetState{state};
                const size_t maxTrans = state.forRDFS ? state.lastTrans + 1 : _net._transitions.size();
                for (auto tid = state.lastTrans; tid < maxTrans; tid++) {
                    auto bid = tid == state.lastTrans ? state.lastBinding : 0;
                    const auto totalBindings = _net._transitions[tid].validVariables.second;
                    if (totalBindings == 0) {
                        if (bid != 0) {
                            continue;
                        }
                        const auto binding = Binding{};
                        if (check(state.marking, tid, binding)) {
                            _fire(newState.marking, tid, binding);
                            newState.lastBinding = 0;
                            newState.lastTrans = 0;
                            state.lastBinding++;
                            state.lastTrans = tid;
                            return newState;
                        }
                    } else {
                        //Checking whether its impossible before iterating through bindings
                        if (!checkInhibitor(state.marking, tid)) {
                            continue;
                        }
                        if (bid == 0) {
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
                            } else if (check == CheckingBool::NEVERTRUE) {
                                continue;
                            }
                        }
                        while (++bid <= totalBindings) {
                            const auto binding = getBinding(tid, bid);
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

            // SuccessorGenerator but only considers current transition
            // If returned successor has trans and binding == uint_max it means the state is fully explored
            // If returned successor has trans == uint_max it means the transition is fully explored
            ColoredPetriNetState _nextOneTrans(ColoredPetriNetState &state) const {
                if (state.lastTrans >= _net._transitions.size()) {
                    auto newState = ColoredPetriNetState{state};
                    newState.lastTrans = std::numeric_limits<uint32_t>::max();
                    newState.lastBinding = std::numeric_limits<uint32_t>::max();
                    return newState;
                }
                return _next(state);
            }
        };
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_H */