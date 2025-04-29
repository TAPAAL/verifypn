#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "IntegerPackCodec.h"
#include "PossibleValues.h"
#include "../ColoredPetriNet.h"
#include "../ColoredPetriNetState.h"
#include <limits>
#include <utils/MathExt.h>

namespace PetriEngine::ExplicitColored {
    struct ConstraintData {
        IntegerPackCodec<size_t, Color_t> stateCodec;
        std::vector<Variable_t> variableIndex;
        std::vector<PossibleValues> possibleVariableValues;
    };

    struct TraceMapStep {
        uint64_t id;
        uint64_t predecessorId;
        Transition_t transition;
        Binding_t binding;
    };

    class ColoredSuccessorGenerator {
    public:
        explicit ColoredSuccessorGenerator(const ColoredPetriNet& net);
        ~ColoredSuccessorGenerator() = default;

        std::pair<ColoredPetriNetStateFixed, TraceMapStep> next(ColoredPetriNetStateFixed& state) const {
            return _nextFixed(state);
        }

        std::pair<ColoredPetriNetStateEven, TraceMapStep> next(ColoredPetriNetStateEven& state) const {
            return _nextEven(state);
        }

        [[nodiscard]] const ColoredPetriNet& net() const {
            return _net;
        }

        Binding_t findNextValidBinding(const ColoredPetriNetMarking& marking, Transition_t tid, Binding_t bid, uint64_t totalBindings, Binding& binding, size_t stateId) const;

        void shrinkState(const size_t stateId) const {
            const auto lower = _constraintData.lower_bound(_getKey(stateId, 0));
            const auto upper = _constraintData.upper_bound(_getKey(stateId, 0xFFFF));
            _constraintData.erase(lower, upper);
        }
        void getBinding(Transition_t tid, Binding_t bid, Binding& binding) const;
        void fire(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
    protected:
        [[nodiscard]] bool check(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        [[nodiscard]] bool checkInhibitor(const ColoredPetriNetMarking& state, Transition_t tid) const;
        [[nodiscard]] bool checkPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
    private:
        mutable std::map<size_t, ConstraintData> _constraintData;
        mutable size_t _nextId = 1;
        const ColoredPetriNet& _net;
        std::map<size_t, ConstraintData>::iterator _calculateConstraintData(const ColoredPetriNetMarking& marking, size_t id, Transition_t transition, bool& noPossibleBinding) const;
        [[nodiscard]] bool _hasMinimalCardinality(const ColoredPetriNetMarking& marking, Transition_t tid) const;
        [[nodiscard]] bool _shouldEarlyTerminateTransition(const ColoredPetriNetMarking& marking, const Transition_t tid) const {
            if (!checkInhibitor(marking, tid)) {
                return true;
            }
            if (!_hasMinimalCardinality(marking, tid)) {
                return true;
            }
            return false;
        }

        std::pair<ColoredPetriNetStateFixed, TraceMapStep> _nextFixed(ColoredPetriNetStateFixed &state) const {
            const auto& tid = state.getCurrentTransition();
            const auto& bid = state.getCurrentBinding();
            Binding binding;
            while (tid < _net.getTransitionCount()) {
                const auto totalBindings = _net._transitions[state.getCurrentTransition()].totalBindings;
                const auto nextBid = findNextValidBinding(state.marking, tid, bid, totalBindings, binding, state.id);
                if (nextBid != std::numeric_limits<Binding_t>::max()) {
                    auto newState = ColoredPetriNetStateFixed(state.marking);
                    newState.id = _nextId++;
                    fire(newState.marking, tid, binding);
                    state.nextBinding(nextBid);
                    return std::make_pair(newState, TraceMapStep {
                        newState.id,
                        state.id,
                        tid,
                        nextBid
                    });
                }
                state.nextTransition();
            }
            state.setDone();
            return std::make_pair(ColoredPetriNetStateFixed{{}}, TraceMapStep {});
        }

        // SuccessorGenerator but only considers current transition
        std::pair<ColoredPetriNetStateEven, TraceMapStep> _nextEven(ColoredPetriNetStateEven &state) const {
            auto [tid, bid] = state.getNextPair();
            auto totalBindings = _net._transitions[tid].totalBindings;
            Binding binding;
            //If bid is updated at the end optimizations seem to make the loop not work
            while (bid != std::numeric_limits<Binding_t>::max()) {
                const auto nextBid = findNextValidBinding(state.marking, tid, bid, totalBindings, binding, state.id);
                state.updatePair(tid, nextBid);
                if (nextBid != std::numeric_limits<Binding_t>::max()) {
                    auto newState = ColoredPetriNetStateEven{state, _net.getTransitionCount()};
                    newState.id = _nextId++;
                    fire(newState.marking, tid, binding);

                    return std::make_pair(newState, TraceMapStep {
                        newState.id,
                        state.id,
                        tid,
                        nextBid
                    });
                }
                std::tie(tid, bid) = state.getNextPair();
                totalBindings = _net._transitions[tid].totalBindings;
            }
            return std::make_pair(ColoredPetriNetStateEven {{}, 0}, TraceMapStep {});
        }

        [[nodiscard]] static uint64_t _getKey(const size_t stateId, const Transition_t transition) {
            return ((stateId & 0xFFFF'FFFF'FFFF) << 16) | ((static_cast<uint64_t>(transition) & 0xFFFF));
        }
    };
}

#endif /* COLOREDSUCCESSORGENERATOR_H */