#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include <utility>

#include "ColoredPetriNetMarking.h"
namespace PetriEngine{
    namespace ExplicitColored{

        struct ColoredPetriNetState{
            ColoredPetriNetState(const ColoredPetriNetState& oldState) : marking(oldState.marking){};
            explicit ColoredPetriNetState(ColoredPetriNetMarking marking) : marking(std::move(marking)) {};
            ColoredPetriNetState(ColoredPetriNetState&&) = default;
            ColoredPetriNetState& operator=(const ColoredPetriNetState&) = default;
            ColoredPetriNetState& operator=(ColoredPetriNetState&&) = default;

            std::pair<Transition_t, Binding_t> getNextPair() {
                return {_currentTransition,_currentBinding};
            }

            void updatePair(const Transition_t tid, const Binding_t bid) {
                if (tid != std::numeric_limits<Transition_t>::max()) {
                    _currentBinding = bid;
                    _currentTransition = tid;
                }else {
                    shuffle = true;
                    done = true;
                }
            }
            
            void shrink() {
                marking.shrink();
            }

            ColoredPetriNetMarking marking;
            bool shuffle = false;
            bool done = false;
            private:
                Binding_t _currentBinding = 0;
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
            std::pair<Transition_t, Binding_t> getNextPair() {
                Transition_t tid;
                Binding_t bid;
                if (_completedTransitions == _map.size()) {
                    tid = std::numeric_limits<Transition_t>::max();
                    bid = std::numeric_limits<Binding_t>::max();
                }
                else if (_map.size() <= _currentIndex) {
                    _currentIndex = 0;
                    tid = std::numeric_limits<Transition_t>::max();
                    bid = std::numeric_limits<Binding_t>::max();
                    shuffle = true;
                }
                else {
                    auto binding = _map[_currentIndex];
                    while (binding == std::numeric_limits<Binding_t>::max() && _currentIndex + 1 < _map.size()) {
                        ++_currentIndex;
                        binding = _map[_currentIndex];
                    }
                    tid = _currentIndex;
                    bid = binding;
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
                            done = true;
                        }else {
                            skip = true;
                        }
                    }
                }
            }

            void shrink() {
                marking.shrink();
            }

            ColoredPetriNetMarking marking;
            bool done = false;
            bool shuffle = false;
            bool skip = false;
        private:
            std::vector<Binding_t> _map;
            uint32_t _currentIndex = 0;
            uint32_t _completedTransitions = 0;

        };
    }
}
#endif //COLOREDPETRINETSTATE_H