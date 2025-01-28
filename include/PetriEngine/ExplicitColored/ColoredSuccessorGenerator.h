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
            explicit ColoredSuccessorGenerator(const ColoredPetriNet& net, const size_t seed);
            ~ColoredSuccessorGenerator();

            ColoredPetriNetState next(ColoredPetriNetState& state) const {
                return _next(state);
            }

            ColoredPetriNetStateOneTrans next(ColoredPetriNetStateOneTrans& state) const {
                return _nextOneTrans(state);
            }

            ColoredPetriNetStateRandom next(ColoredPetriNetStateRandom& state) {
                return _next(state);
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

            ColoredPetriNetStateRandom getInitialStateRandom(ColoredPetriNetMarking marking);
        protected:
            const ColoredPetriNet& _net;
            std::default_random_engine _random_engine;
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
                while (bid != std::numeric_limits<Binding_t>::max()) {
                    const auto totalBindings = _net._transitions[tid].validVariables.second;
                    if (totalBindings == 0) {
                        if (bid == 0) {
                            const auto binding = Binding{};
                            if (check(state.marking, tid, binding)) {
                                auto newState = ColoredPetriNetStateOneTrans{state, _net.getTransitionCount()};
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
                                    auto newState = ColoredPetriNetStateOneTrans{state, _net.getTransitionCount()};
                                    _fire(newState.marking, tid, binding);
                                    state.updatePair(tid,bid);
                                    return newState;
                                }
                                if (check == CheckingBool::NEVERTRUE) {
                                    state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                                    auto [nextTid, nextBid] = state.getNextPair();
                                    tid = nextTid;
                                    bid = nextBid;
                                    continue;
                                }
                            }
                            while (++bid <= totalBindings) {
                                const auto binding = getBinding(tid, bid);
                                if (checkPresetAndGuard(state.marking, tid, binding)) {
                                    auto newState = ColoredPetriNetStateOneTrans{state, _net.getTransitionCount()};
                                    _fire(newState.marking, tid, binding);
                                    state.updatePair(tid, bid);
                                    return newState;
                                }
                            }
                        }
                    }
                    state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                    auto [nextTid, nextBid] = state.getNextPair();
                    tid = nextTid;
                    bid = nextBid;
                }
            return {{},0};
            }

            [[nodiscard]] ColoredPetriNetStateRandom createNewRandomState(ColoredPetriNetMarking previousMarking) {
                std::deque<Transition_t> transitions(_net.getTransitionCount());
                std::iota(transitions.begin(), transitions.end(), 0);
                std::shuffle(transitions.begin(), transitions.end(), _random_engine);
                return ColoredPetriNetStateRandom{std::move(previousMarking), std::queue(std::move(transitions))};
            }

            bool shouldEarlyTerminateTransition(const ColoredPetriNetStateRandom& state) {
                if (!checkInhibitor(state.marking, state.getCurrentTransition()))
                    return true;
                if (!hasMinimalCardinality(state.marking, state.getCurrentTransition()))
                    return true;
                if (_net._transitions[state.getCurrentTransition()].guardExpression != nullptr && _net._transitions[state.getCurrentTransition()].variables.empty()) {
                    return true;
                }
                return false;
            }

            ColoredPetriNetStateRandom _next(ColoredPetriNetStateRandom &state) {
                while (state.getCurrentTransition() < _net.getTransitionCount()) {
                    const auto totalBindings = _net._transitions[state.getCurrentTransition()].validVariables.second;
                    if (state.getCurrentBinding() == 0) {
                        if (totalBindings == 0) {
                            state.nextTransition();
                            if (check(state.marking, state.getCurrentTransition(), Binding{})) {
                                auto newState = createNewRandomState(state.marking);
                                _fire(newState.marking, state.getCurrentTransition(), Binding{});
                                return newState;
                            }
                            continue;
                        }
                        if (shouldEarlyTerminateTransition(state)) {
                            state.nextTransition();
                            continue;
                        }
                    }
                    while (state.getCurrentBinding() <= totalBindings) {
                        const auto binding = getBinding(state.getCurrentTransition(), state.getCurrentBinding());
                        state.nextBinding();
                        if (checkPresetAndGuard(state.marking, state.getCurrentTransition(), binding)) {
                            auto newState = createNewRandomState(state.marking);
                            _fire(newState.marking, state.getCurrentTransition(), binding);
                            return newState;
                        }
                    }

                    state.nextTransition();
                }
                return ColoredPetriNetStateRandom{{}, {}};
            }

        private:
            [[nodiscard]] bool hasMinimalCardinality(const ColoredPetriNetMarking& marking, Transition_t transition) const;
        };
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_H */