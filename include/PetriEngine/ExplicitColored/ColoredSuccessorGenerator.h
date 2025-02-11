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

            ColoredPetriNetStateV2 next(ColoredPetriNetStateV2& state) const {
                return _nextV2(state);
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
            std::default_random_engine _random_engine;
            void _fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;

            ColoredPetriNetState _next(ColoredPetriNetState &state) const {
                while (state.getCurrentTransition() < _net.getTransitionCount()) {
                    const auto totalBindings = _net._transitions[state.getCurrentTransition()].validVariables.second;
                    if (state.getCurrentBinding() == 1) {
                        if (totalBindings == 0) {
                            if (check(state.marking, state.getCurrentTransition(), Binding{})) {
                                auto newState = ColoredPetriNetState(state.marking);
                                _fire(newState.marking, state.getCurrentTransition(), Binding{});
                                state.nextTransition();
                                return newState;
                            }
                            state.nextTransition();
                            continue;
                        }
                        if (shouldEarlyTerminateTransition(state.marking, state.getCurrentTransition())) {
                            state.nextTransition();
                            continue;
                        }
                    }
                    while (state.getCurrentBinding() <= totalBindings) {
                        const auto binding = getBinding(state.getCurrentTransition(), state.getCurrentBinding());
                        state.nextBinding();
                        if (checkPresetAndGuard(state.marking, state.getCurrentTransition(), binding)) {
                            auto newState = ColoredPetriNetState(state.marking);
                            _fire(newState.marking, state.getCurrentTransition(), binding);
                            return newState;
                        }
                    }
                    state.nextTransition();
                }
                state.setDone();
                return ColoredPetriNetState{{}};
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
                        if (bid == 0 && shouldEarlyTerminateTransition(state.marking, tid)) {
                            state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                            auto [nextTid, nextBid] = state.getNextPair();
                            tid = nextTid;
                            bid = nextBid;
                            continue;
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
                    state.updatePair(tid, std::numeric_limits<Binding_t>::max());
                    auto [nextTid, nextBid] = state.getNextPair();
                    tid = nextTid;
                    bid = nextBid;
                }
                return {{},0};
            }

            ColoredPetriNetStateV2 _nextV2(ColoredPetriNetStateV2 &state) const {
transitionLoop:
                while (state.getCurrentTransition() < _net.getTransitionCount()) {
                    if (state.shouldCheckEarlyTermination()) {
                        const auto totalBindings = _net._transitions[state.getCurrentTransition()].validVariables.second;
                        if (totalBindings == 0) {
                            if (check(state.marking, state.getCurrentTransition(), Binding{})) {
                                auto newState = ColoredPetriNetStateV2(state.marking);
                                _fire(newState.marking, state.getCurrentTransition(), Binding{});
                                state.nextTransition();
                                return newState;
                            }
                            state.nextTransition();
                            continue;
                        }

                        if (shouldEarlyTerminateTransition(state.marking, state.getCurrentTransition())) {
                            state.nextTransition();
                            continue;
                        }
                    }

                    std::set<Variable_t> allVariables;
                    std::set<Variable_t> inputArcVariables;
                    _net.getInputVariables(state.getCurrentTransition(), allVariables);
                    _net.getInputVariables(state.getCurrentTransition(), inputArcVariables);
                    _net.getOutputVariables(state.getCurrentTransition(), allVariables);
                    _net.getGuardVariables(state.getCurrentTransition(), allVariables);
                    std::vector<std::pair<Variable_t, std::vector<Color_t>>> variableValues;
                    for (Variable_t variable : allVariables) {
                        if (inputArcVariables.find(variable) == inputArcVariables.end()) {
                            variableValues.emplace_back(variable, std::vector { ALL_COLOR });
                        }
                        std::vector<Color_t> possibleColors = {ALL_COLOR};
                        for (
                            auto i = _net._transitionArcs[state.getCurrentTransition()].first;
                            i < _net._transitionArcs[state.getCurrentTransition()].second;
                            i++
                        ) {
                            auto& arc = _net._arcs[i];
                            const auto& arcVariables = arc.expression->getVariables();
                            if (arcVariables.find(variable) == arcVariables.end()) {
                                continue;
                            }
                            auto arcPossibleColors = arc.expression->getPossibleBindings(variable, state.marking.markings[arc.from]);
                            if (arcPossibleColors.find(ALL_COLOR) != arcPossibleColors.end()) {
                                continue;
                            }
                            if (std::binary_search(possibleColors.begin(), possibleColors.end(), ALL_COLOR)) {
                                possibleColors = std::vector(arcPossibleColors.begin(), arcPossibleColors.end());
                            } else {
                                std::vector<Color_t> newPossibleColors;
                                std::set_intersection(possibleColors.begin(), possibleColors.end(), arcPossibleColors.begin(), arcPossibleColors.end(), std::back_inserter(newPossibleColors));
                                possibleColors = std::move(newPossibleColors);
                            }
                        }
                        variableValues.emplace_back(variable, std::move(possibleColors));
                    }

                    if (state.shouldCheckEarlyTermination()) {
                        state.variableIndices.clear();
                        for (const auto& variableValue : variableValues) {
                            if (variableValue.second.empty()) {
                                state.nextTransition();
                                goto transitionLoop;
                            }
                            state.variableIndices.push_back(0);
                        }
                    }

                    std::vector<size_t> stateMaxes;
                    for (const auto& variableValue : variableValues) {
                        if (std::binary_search(variableValue.second.begin(), variableValue.second.end(), ALL_COLOR)) {
                            stateMaxes.push_back(_net._variables[variableValue.first].colorType->colors);
                        } else {
                            stateMaxes.push_back(variableValue.second.size());
                        }
                    }
                    state.checkedEarlyTermination();
                    Binding binding {};
                    while (true) {
                        for (auto variableIndex = 0; variableIndex < variableValues.size(); ++variableIndex) {
                            if (std::binary_search(variableValues[variableIndex].second.begin(), variableValues[variableIndex].second.end(), ALL_COLOR)) {
                                binding.setValue(
                                    variableValues[variableIndex].first,
                                    state.variableIndices[variableIndex]
                                );
                            } else {
                                binding.setValue(
                                   variableValues[variableIndex].first,
                                   variableValues[variableIndex].second[state.variableIndices[variableIndex]]
                               );
                            }
                        }
                        if (checkPresetAndGuard(state.marking, state.getCurrentTransition(), binding)) {
                            auto newState = ColoredPetriNetStateV2{state.marking};
                            _fire(newState.marking, state.getCurrentTransition(), binding);
                            if (state.incrementVariableIndices(stateMaxes)) {
                                state.nextTransition();
                            }
                            return newState;
                        }
                        if (state.incrementVariableIndices(stateMaxes)) {
                            state.nextTransition();
                            goto transitionLoop;
                        }
                    }
                }
                state.setDone();
                return ColoredPetriNetStateV2{{}};
            }

            [[nodiscard]] bool shouldEarlyTerminateTransition(const ColoredPetriNetMarking& marking, const Transition_t transition) const {
                if (!checkInhibitor(marking, transition))
                    return true;
                if (!hasMinimalCardinality(marking, transition))
                    return true;
                return false;
            }

        private:
            [[nodiscard]] bool hasMinimalCardinality(const ColoredPetriNetMarking& marking, Transition_t transition) const;
        };
    }
}
#endif /* COLOREDSUCCESSORGENERATOR_H */