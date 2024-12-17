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
                return _next(state);
            }

            ColoredPetriNetStateOneTrans next(ColoredPetriNetStateOneTrans& state) const {
                return _nextOneTrans(state);
            }

            const ColoredPetriNet& net() const {
                return _net;
            }

            Binding getBinding(Transition_t tid, Binding_t bid) const;
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
                const auto [nextTid, nextBid] = state.getNextPair();
                for (auto tid = nextTid;tid < _net._transitions.size(); tid++) {
                    auto bid = tid == nextTid ? nextBid : 0;
                    const auto totalBindings = _net._transitions[tid].validVariables.second;
                    if (totalBindings == 0) {
                        if (bid != 0) {
                            continue;
                        }
                        const auto binding = Binding{};
                        if (check(state.marking, tid, binding)) {
                            _fire(newState.marking, tid, binding);
                            state.updatePair(tid, ++bid);
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
                            const auto check = firstCheckPresetAndGuard(state.marking, tid, binding);
                            if (check == CheckingBool::TRUE) {
                                _fire(newState.marking, tid, binding);
                                state.updatePair(tid, bid);
                                return newState;
                            } else if (check == CheckingBool::NEVERTRUE) {
                                continue;
                            }
                        }
                        while (++bid <= totalBindings) {
                            const auto binding = getBinding(tid, bid);
                            if (checkPresetAndGuard(state.marking, tid, binding)) {
                                _fire(newState.marking, tid, binding);
                                state.updatePair(tid, bid);

                                return newState;
                            }
                        }
                    }
                }
                state.updatePair(std::numeric_limits<Transition_t>::max(), std::numeric_limits<Binding_t>::max());
                return newState;
            }

            // SuccessorGenerator but only considers current transition
            ColoredPetriNetStateOneTrans _nextOneTrans(ColoredPetriNetStateOneTrans &state) const {
                auto [tid, bid] = state.getNextPair();
                if (bid == std::numeric_limits<Binding_t>::max()) {
                    state.skip = true;
                    return {{},0};
                }
                auto newState = ColoredPetriNetStateOneTrans{state, _net.nTransitions()};
                    const auto totalBindings = _net._transitions[tid].validVariables.second;
                    if (totalBindings == 0) {
                        if (bid == 0) {
                            const auto binding = Binding{};
                            if (check(state.marking, tid, binding)) {
                                _fire(newState.marking, tid, binding);
                                state.updatePair(tid, ++bid);
                                return newState;
                            }
                        }
                    } else {
                        //Checking whether its impossible before iterating through bindings
                        if (checkInhibitor(state.marking, tid)) {
                            if (bid == 0) {
                                bid++;
                                const auto binding = getBinding(tid, bid);
                                const auto check = firstCheckPresetAndGuard(state.marking, tid, binding);
                                if (check == CheckingBool::TRUE) {
                                    _fire(newState.marking, tid, binding);
                                    state.updatePair(tid,bid);
                                    return newState;
                                }
                                if (check == CheckingBool::NEVERTRUE) {
                                    state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                                    return newState;
                                }
                            }
                            while (++bid <= totalBindings) {
                                const auto binding = getBinding(tid, bid);
                                if (checkPresetAndGuard(state.marking, tid, binding)) {
                                    _fire(newState.marking, tid, binding);
                                    state.updatePair(tid, bid);
                                    return newState;
                                }
                            }
                        }
                    }
                state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                return newState;
            }
        };
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_H */