#ifndef COLOREDSUCCESSORGENERATOR_H
#define COLOREDSUCCESSORGENERATOR_H

#include "ColoredPetriNet.h"
#include "ColoredPetriNetState.h"
#include <limits>

namespace PetriEngine::ExplicitColored {
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
        ~ColoredSuccessorGenerator() = default;

        ColoredPetriNetState next(ColoredPetriNetState& state) const {
            return _next(state);
        }

        ColoredPetriNetStateOneTrans next(ColoredPetriNetStateOneTrans& state) const {
            return _nextOneTrans(state);
        }

        [[nodiscard]] const ColoredPetriNet& net() const {
            return _net;
        }

        [[nodiscard]] Binding getBinding(Transition_t tid, Binding_t bid) const;
        [[nodiscard]] bool check(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        [[nodiscard]] bool checkInhibitor(const ColoredPetriNetMarking& state, Transition_t tid) const;
        [[nodiscard]] bool checkPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        [[nodiscard]] CheckingBool firstCheckPresetAndGuard(const ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void consumePreset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
        void producePostset(ColoredPetriNetMarking& state, Transition_t tid, const Binding& binding) const;
    protected:
        const ColoredPetriNet& _net;
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
#endif /* COLOREDSUCCESSORGENERATOR_H */
