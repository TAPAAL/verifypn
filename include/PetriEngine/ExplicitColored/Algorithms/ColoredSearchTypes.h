#ifndef COLORED_SEARCH_TYPES_H
#define COLORED_SEARCH_TYPES_H

namespace PetriEngine {
    namespace ExplicitColored {
        class DFSStructure {
        public:
            ColoredPetriNetState& next() {
                return waiting.top();
            }

            void remove() {
                waiting.pop();
            }

            void add(ColoredPetriNetState state) {
                waiting.emplace(std::move(state));
            }

            bool empty() const {
                return waiting.empty();
            }

            uint32_t size() const {
                return waiting.size();
            }
        private:
            std::stack<ColoredPetriNetState> waiting;
        };

        class BFSStructure {
        public:
            ColoredPetriNetState& next() {
                return waiting.front();
            }

            void remove() {
                waiting.pop();
            }

            void add(ColoredPetriNetState state) {
                waiting.emplace(std::move(state));
            }

            bool empty() const {
                return waiting.empty();
            }

            uint32_t size() const {
                return waiting.size();
            }
        private:
            std::queue<ColoredPetriNetState> waiting;
        };

        class RDFSStructure {
        public:
            explicit RDFSStructure(size_t seed) : _has_removed(true), _rng(seed) {}

            ColoredPetriNetState& next() {
                if (!_has_removed || _cache.empty()) {
                    return _stack.top();
                }

                _has_removed = false;
                std::shuffle(_cache.begin(), _cache.end(), _rng);

                for (auto it = _cache.begin(); _cache.end() != it; ++it) {
                    _stack.emplace(std::move(*it));
                }

                _cache.clear();
                return _stack.top();
            }

            void remove() {
                _has_removed = true;
                _stack.pop();
            }

            void add(ColoredPetriNetState state) {
                _cache.push_back(std::move(state));
            }

            bool empty() const {
                return _stack.empty() && _cache.empty();
            }

            uint32_t size() const {
                return _stack.size() + _cache.size();
            }
        private:
            std::stack<ColoredPetriNetState> _stack;
            std::vector<ColoredPetriNetState> _cache;
            bool _has_removed = false;
            std::default_random_engine _rng;

        };
    }
}

#endif