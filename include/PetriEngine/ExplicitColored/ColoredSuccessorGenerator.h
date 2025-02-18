#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>

namespace PetriEngine::ExplicitColored {
    struct TransitionVariables{
        std::map<Variable_t, std::vector<uint32_t>> possibleValues = std::map<Variable_t, std::vector<uint32_t>>{};
        uint32_t totalBindings = 0;
    };

    class ColoredSuccessorGenerator {
    public:
        explicit ColoredSuccessorGenerator(const ColoredPetriNet& net);
        ~ColoredSuccessorGenerator() = default;

        ColoredPetriNetStateFixed next(ColoredPetriNetStateFixed& state) const {
            return _nextFixed(state);
        }

        ColoredPetriNetStateEven next(ColoredPetriNetStateEven& state) const {
            return _nextEven(state);
        }

        ColoredPetriNetStateConstrained next(ColoredPetriNetStateConstrained& state) const {
            return _nextV2(state);
        }

        [[nodiscard]] const ColoredPetriNet& net() const {
            return _net;
        }
        [[nodiscard]] Binding_t findNextValidBinding(const ColoredPetriNetMarking& marking, Transition_t tid, Binding_t bid, uint64_t totalBindings, Binding& binding) const;
    protected:
        void getBinding(Transition_t tid, Binding_t bid, Binding& binding) const;
        [[nodiscard]] bool check(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        [[nodiscard]] bool checkInhibitor(const ColoredPetriNetMarking& state, Transition_t tid) const;
        [[nodiscard]] bool checkPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
    private:
        const ColoredPetriNet& _net;
        void _fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        [[nodiscard]] bool _hasMinimalCardinality(const ColoredPetriNetMarking& marking, Transition_t tid) const;
        [[nodiscard]] bool _shouldEarlyTerminateTransition(const ColoredPetriNetMarking& marking, const Transition_t tid) const {
            if (!checkInhibitor(marking, tid))
                return true;
            if (!_hasMinimalCardinality(marking, tid))
                return true;
            return false;
        }

        ColoredPetriNetStateFixed _nextFixed(ColoredPetriNetStateFixed &state) const {
            const auto& tid = state.getCurrentTransition();
            const auto& bid = state.getCurrentBinding();
            Binding binding;
            while (tid < _net.getTransitionCount()) {
                const auto totalBindings = _net._transitions[state.getCurrentTransition()].validVariables.second;
                const auto nextBid = findNextValidBinding(state.marking, tid, bid, totalBindings, binding);
                if (nextBid != std::numeric_limits<Binding_t>::max()) {
                    auto newState = ColoredPetriNetStateFixed(state.marking);
                    _fire(newState.marking, tid, binding);
                    state.nextBinding(nextBid);
                    return newState;
                }
                state.nextTransition();
            }
            state.setDone();
            return ColoredPetriNetStateFixed{{}};
        }

        // SuccessorGenerator but only considers current transition
        ColoredPetriNetStateEven _nextEven(ColoredPetriNetStateEven &state) const {
            auto [tid, bid] = state.getNextPair();
            auto totalBindings = _net._transitions[tid].validVariables.second;
            Binding binding;
            //If bid is updated at the end optimizations seem to make the loop not work
            while (bid != std::numeric_limits<Binding_t>::max()){
                {
                    const auto nextBid = findNextValidBinding(state.marking, tid, bid, totalBindings, binding);
                    state.updatePair(tid, nextBid);
                    if (nextBid != std::numeric_limits<Binding_t>::max()) {
                        auto newState = ColoredPetriNetStateEven{state, _net.getTransitionCount()};
                        _fire(newState.marking, tid, binding);
                        return newState;
                    }
                }
                auto [nextTid, nextBid] = state.getNextPair();
                tid = nextTid;
                bid = nextBid;
                totalBindings = _net._transitions[tid].validVariables.second;
            }
            return {{},0};
        }

        [[nodiscard]] bool shouldEarlyTerminateTransition(const ColoredPetriNetMarking& marking, const Transition_t transition) const {
            if (!checkInhibitor(marking, transition))
                return true;
            if (!_hasMinimalCardinality(marking, transition))
                return true;
            return false;
        }

        ColoredPetriNetStateConstrained _nextV2(ColoredPetriNetStateConstrained &state) const {
transitionLoop:
            while (state.getCurrentTransition() < _net.getTransitionCount()) {
                if (state.shouldCheckEarlyTermination()) {
                    const auto totalBindings = _net._transitions[state.getCurrentTransition()].validVariables.second;
                    if (totalBindings == 0) {
                        if (check(state.marking, state.getCurrentTransition(), Binding{})) {
                            auto newState = ColoredPetriNetStateConstrained(state.marking);
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

                if (state.shouldCheckEarlyTermination()) {
                    std::set<Variable_t> allVariables;
                    std::set<Variable_t> inputArcVariables;
                    _net.getInputVariables(state.getCurrentTransition(), allVariables);
                    _net.getInputVariables(state.getCurrentTransition(), inputArcVariables);
                    _net.getOutputVariables(state.getCurrentTransition(), allVariables);
                    _net.getGuardVariables(state.getCurrentTransition(), allVariables);
                    for (Variable_t variable : allVariables) {
                        if (inputArcVariables.find(variable) == inputArcVariables.end()) {
                            state.variableValues.emplace_back(variable, std::vector { ALL_COLOR });
                            continue;
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
                        state.variableValues.emplace_back(variable, std::move(possibleColors));
                    }
                    for (const auto& variableValue : state.variableValues) {
                        if (variableValue.second.empty()) {
                            state.nextTransition();
                            goto transitionLoop;
                        }
                        state.variableIndices.push_back(0);
                    }
                    for (const auto&[variableIndex, variableValues] : state.variableValues) {
                        if (std::binary_search(variableValues.begin(), variableValues.end(), ALL_COLOR)) {
                            state.stateMaxes.push_back(_net._variables[variableIndex].colorType->colors);
                        } else {
                            state.stateMaxes.push_back(variableValues.size());
                        }
                    }
                }

                state.checkedEarlyTermination();
                Binding binding {};
                while (true) {
                    for (auto variableIndex = 0; variableIndex < state.variableValues.size(); ++variableIndex) {
                        if (std::binary_search(state.variableValues[variableIndex].second.begin(), state.variableValues[variableIndex].second.end(), ALL_COLOR)) {
                            binding.setValue(
                                state.variableValues[variableIndex].first,
                                state.variableIndices[variableIndex]
                            );
                        } else {
                            binding.setValue(
                               state.variableValues[variableIndex].first,
                               state.variableValues[variableIndex].second[state.variableIndices[variableIndex]]
                           );
                        }
                    }
                    if (checkPresetAndGuard(state.marking, state.getCurrentTransition(), binding)) {
                        auto newState = ColoredPetriNetStateConstrained{state.marking};
                        _fire(newState.marking, state.getCurrentTransition(), binding);
                        if (state.incrementVariableIndices(state.stateMaxes)) {
                            state.nextTransition();
                        }
                        return newState;
                    }
                    if (state.incrementVariableIndices(state.stateMaxes)) {
                        state.nextTransition();
                        goto transitionLoop;
                    }
                }
            }
            state.setDone();
            return ColoredPetriNetStateConstrained{{}};
        }
    };
}

#endif /* COLOREDSUCCESSORGENERATOR_H */