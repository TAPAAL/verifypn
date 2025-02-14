#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include <queue>
#include <utility>

#include "ColoredPetriNetMarking.h"

namespace PetriEngine::ExplicitColored{

    struct ColoredPetriNetState{
        explicit ColoredPetriNetState(ColoredPetriNetMarking marking) : marking(std::move(marking)) {};
        ColoredPetriNetState(const ColoredPetriNetState& oldState) = default;
        ColoredPetriNetState(ColoredPetriNetState&&) = default;
        ColoredPetriNetState& operator=(const ColoredPetriNetState&) = default;
        ColoredPetriNetState& operator=(ColoredPetriNetState&&) = default;

        void shrink() {
            marking.shrink();
        }

        void setDone() {
            _done = true;
        }

        [[nodiscard]] bool done() const {
            return _done;
        }

        [[nodiscard]] Transition_t getCurrentTransition() const {
            return _currentTransition;
        }

        [[nodiscard]] Binding_t getCurrentBinding() const {
            return _currentBinding;
        }

        void nextTransition() {
            _currentTransition += 1;
            _currentBinding = 1;
        }

        void nextBinding() {
            _currentBinding += 1;
        }

        ColoredPetriNetMarking marking;
    private:
        bool _done = false;

        Binding_t _currentBinding = 1;
        Transition_t _currentTransition = 0;
    };

    struct ColoredPetriNetStateOneTrans {
        ColoredPetriNetStateOneTrans(const ColoredPetriNetStateOneTrans& state) : marking(state.marking), _map(state._map), _currentIndex(state._currentIndex) {};
        ColoredPetriNetStateOneTrans(const ColoredPetriNetStateOneTrans& oldState, const size_t& numberOfTransitions) : marking(oldState.marking) {
            _map = std::vector<Binding_t>(numberOfTransitions);
        }
        ColoredPetriNetStateOneTrans(ColoredPetriNetMarking marking, const size_t& numberOfTransitions) : marking(std::move(marking)){
            _map = std::vector<Binding_t>(numberOfTransitions);
        }
        ColoredPetriNetStateOneTrans(ColoredPetriNetStateOneTrans&& state) = default;

        ColoredPetriNetStateOneTrans& operator=(const ColoredPetriNetStateOneTrans&) = default;
        ColoredPetriNetStateOneTrans& operator=(ColoredPetriNetStateOneTrans&&) = default;
        std::pair<Transition_t, Binding_t> getNextPair() {
            Transition_t tid = _currentIndex;
            Binding_t bid = std::numeric_limits<Binding_t>::max();
            if (done()) {
                return {tid,bid};
            }
            auto it = _map.begin() + _currentIndex;
            while (it != _map.end() && *it == std::numeric_limits<Transition_t>::max()) {
                ++it;
                ++_currentIndex;
            }
            if (it == _map.end()) {
                _currentIndex = 0;
                shuffle = true;
            }else {
                tid = _currentIndex;
                bid = *it;
                ++_currentIndex;
            }
            return {tid,bid};
        }

        void updatePair(const Transition_t tid, const Binding_t bid) {
            if (tid < _map.size() && bid != _map[tid]) {
                _map[tid] = bid;
                if (bid == std::numeric_limits<Binding_t>::max()) {
                    _completedTransitions += 1;
                    if (_completedTransitions == _map.size()) {
                        _done = true;
                    }
                }
            }
        }

        void shrink() {
            marking.shrink();
        }

        [[nodiscard]] bool done() const {
            return _done;
        }

        ColoredPetriNetMarking marking;
        bool shuffle = false;
    private:
        bool _done = false;
        std::vector<Binding_t> _map;
        uint32_t _currentIndex = 0;
        uint32_t _completedTransitions = 0;
    };
}

#endif //COLOREDPETRINETSTATE_H