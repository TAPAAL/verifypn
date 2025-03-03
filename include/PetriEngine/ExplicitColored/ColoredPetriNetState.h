#ifndef COLOREDPETRINETSTATE_H
#define COLOREDPETRINETSTATE_H

#include <queue>
#include <utility>
#include "PetriEngine/ExplicitColored/StateCodec.h"
#include "ColoredPetriNetMarking.h"

namespace PetriEngine::ExplicitColored {
    struct ColoredPetriNetStateFixed {
        explicit ColoredPetriNetStateFixed(ColoredPetriNetMarking marking) : marking(std::move(marking)) {};
        ColoredPetriNetStateFixed(const ColoredPetriNetStateFixed& oldState) = default;
        ColoredPetriNetStateFixed(ColoredPetriNetStateFixed&&) = default;
        ColoredPetriNetStateFixed& operator=(const ColoredPetriNetStateFixed&) = default;
        ColoredPetriNetStateFixed& operator=(ColoredPetriNetStateFixed&&) = default;

        void shrink() {
            marking.shrink();
        }

        void setDone() {
            _done = true;
        }

        [[nodiscard]] bool done() const {
            return _done;
        }

        [[nodiscard]] const Transition_t& getCurrentTransition() const {
            return _currentTransition;
        }

        [[nodiscard]] const Binding_t& getCurrentBinding() const {
            return _currentBinding;
        }

        void nextTransition() {
            _currentTransition += 1;
            _currentBinding = 0;
        }

        void nextBinding() {
            _currentBinding += 1;
        }

        void nextBinding(const Binding_t bid) {
            _currentBinding = bid + 1;
        }

        ColoredPetriNetMarking marking;
        size_t id;
    private:
        bool _done = false;

        Binding_t _currentBinding = 0;
        Transition_t _currentTransition = 0;
    };

    struct ColoredPetriNetStateEven {
        ColoredPetriNetStateEven(const ColoredPetriNetStateEven& oldState, const size_t& numberOfTransitions) : marking(oldState.marking) {
            _map = std::vector<Binding_t>(numberOfTransitions);
        }
        ColoredPetriNetStateEven(ColoredPetriNetMarking marking, const size_t& numberOfTransitions) : marking(std::move(marking))  {
            _map = std::vector<Binding_t>(numberOfTransitions);
        }
        ColoredPetriNetStateEven(ColoredPetriNetStateEven&& state) = default;
        ColoredPetriNetStateEven(const ColoredPetriNetStateEven& state) = default;
        ColoredPetriNetStateEven& operator=(const ColoredPetriNetStateEven&) = default;
        ColoredPetriNetStateEven& operator=(ColoredPetriNetStateEven&&) = default;
        std::pair<Transition_t, Binding_t> getNextPair() {
            Transition_t tid = _currentIndex;
            Binding_t bid = std::numeric_limits<Binding_t>::max();
            if (done()) {
                return {tid,bid};
            }
            auto it = _map.begin() + _currentIndex;
            while (it != _map.end() && *it == std::numeric_limits<Binding_t>::max()) {
                ++it;
                ++_currentIndex;
            }
            if (it == _map.end()) {
                _currentIndex = 0;
                shuffle = true;
            } else {
                tid = _currentIndex;
                bid = *it;
                ++_currentIndex;
            }
            return {tid,bid};
        }

        //Takes last fired transition-binding pair, so next binding is bid + 1
        void updatePair(const Transition_t tid, const Binding_t bid) {
            if (tid < _map.size()) {
                auto& oldBid = _map[tid];
                if (bid == std::numeric_limits<Binding_t>::max()) {
                    if (bid != _map[tid]) {
                        oldBid = bid;
                        _completedTransitions += 1;
                        if (_completedTransitions == _map.size()) {
                            _done = true;
                        }
                    }
                } else {
                    oldBid = bid + 1;
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
        size_t id;

    private:
        bool _done = false;
        std::vector<Binding_t> _map;
        uint32_t _currentIndex = 0;
        uint32_t _completedTransitions = 0;
    };

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
}

#endif //COLOREDPETRINETSTATE_H