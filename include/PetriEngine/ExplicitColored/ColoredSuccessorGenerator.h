#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>
#include <utils/MathExt.h>

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
        struct PossibleValues {
            explicit PossibleValues(std::vector<Color_t> colors)
                : colors(std::move(colors)), allColors(false) {}

            explicit PossibleValues(const std::set<Color_t>& colors)
                : colors(colors.begin(), colors.end()), allColors(false) {}

            static PossibleValues getAll() {
                PossibleValues rv(std::vector<Color_t> {});
                rv.allColors = true;
                return rv;
            }

            static PossibleValues getEmpty() {
                PossibleValues rv(std::vector<Color_t> {});
                rv.allColors = false;
                return rv;
            }

            void sort() {
                std::sort(colors.begin(), colors.end());
            }

            void intersect(const PossibleValues& other) {
                if (other.allColors) {
                    return;
                }
                if (allColors) {
                    colors = other.colors;
                    return;
                }
                std::vector<Color_t> newColors;
                std::set_intersection(
                    colors.cbegin(),
                    colors.cend(),
                    other.colors.cbegin(),
                    other.colors.cend(),
                    std::back_inserter(newColors)
                );
                colors = std::move(newColors);
            }

            void intersect(const std::set<Color_t>& other) {
                if (allColors) {
                    colors.clear();
                    colors.insert(colors.begin(), other.cbegin(), other.cend());
                    return;
                }

                std::vector<Color_t> newColors;
                std::set_intersection(
                    colors.cbegin(),
                    colors.cend(),
                    other.cbegin(),
                    other.cend(),
                    std::back_inserter(newColors)
                );
                colors = std::move(newColors);
            }

            std::vector<Color_t> colors;
            bool allColors;
        };
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

                //if (state.shouldCheckEarlyTermination()) {
                    std::set<Variable_t> allVariables;
                    std::set<Variable_t> inputArcVariables;
                    _net.getInputVariables(state.getCurrentTransition(), allVariables);
                    _net.getInputVariables(state.getCurrentTransition(), inputArcVariables);
                    _net.getOutputVariables(state.getCurrentTransition(), allVariables);
                    _net.getGuardVariables(state.getCurrentTransition(), allVariables);
                    std::map<Variable_t, PossibleValues> possibleVariableValues;
                    for (Variable_t variable : allVariables) {
                        auto& values = *possibleVariableValues.emplace(variable, PossibleValues::getAll()).first;
                        const auto& constraints = _net._transitions[state.getCurrentTransition()].preplacesVariableConstraints.find(variable);
                        if (constraints == _net._transitions[state.getCurrentTransition()].preplacesVariableConstraints.end()) {
                            continue;
                        }
                        for (const auto& constraint : constraints->second) {
                            const auto& place = state.marking.markings[constraint.place];

                            if (constraint.isTop()) {
                                continue;
                            }

                            std::set<Color_t> possibleColors;

                            if (values.second.allColors) {
                                values.second.colors.reserve(place.counts().size());
                            }

                            for (const auto& tokens : place.counts()) {
                                auto bindingValue = add_color_offset(
                                    tokens.first.getSequence()[constraint.colorIndex],
                                    -constraint.colorOffset,
                                    _net._variables[variable].colorType->colors
                                );

                                if (values.second.allColors) {
                                    values.second.colors.push_back(bindingValue);
                                } else {
                                    possibleColors.insert(bindingValue);
                                }
                            }

                            if (values.second.allColors) {
                                values.second.allColors = false;
                                std::sort(values.second.colors.begin(), values.second.colors.end());
                            } else {
                                values.second.intersect(possibleColors);
                            }
                            if (values.second.colors.empty() && !values.second.allColors) {
                                state.nextTransition();
                                goto transitionLoop;
                            }
                        }
                        state.stateMaxes[variable] = values.second.allColors
                            ? _net._variables[variable].colorType->colors
                            : values.second.colors.size();
                    }
                //}

                state.checkedEarlyTermination();
                Binding binding {};
                while (true) {
                    for (const auto var : allVariables) {
                        const auto& possibleValues = possibleVariableValues.find(var)->second;
                        if (possibleValues.allColors) {
                            binding.setValue(
                                var,
                                state.variableIndices[var]
                            );
                        } else {
                            binding.setValue(
                                var,
                                possibleValues.colors[state.variableIndices[var]]
                            );
                        }
                    }
                    if (checkPresetAndGuard(state.marking, state.getCurrentTransition(), binding)) {
                        auto newState = ColoredPetriNetStateConstrained{state.marking};
                        _fire(newState.marking, state.getCurrentTransition(), binding);
                        if (state.incrementVariableIndices()) {
                            state.nextTransition();
                        }
                        return newState;
                    }
                    if (state.incrementVariableIndices()) {
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