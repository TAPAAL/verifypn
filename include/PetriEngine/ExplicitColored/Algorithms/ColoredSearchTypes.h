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
            explicit RDFSStructure(const size_t seed) : _rng(seed) {}

            ColoredPetriNetStateRandom& next() {
                if (_stack.empty()) {
                    shuffle();
                }
                return _stack.top();
            }

            void remove() {
                _stack.pop();
            }

            void shuffle() {
                std::shuffle(_cache.begin(), _cache.end(), _rng);
                for (auto & it : _cache) {
                    _stack.emplace(std::move(it));
                }
                _cache.clear();
            }

            void add(ColoredPetriNetStateRandom state) {
                _stack.push(std::move(state));
            }

            void addToFrontier(ColoredPetriNetStateRandom state) {
                _cache.push_back(std::move(state));
            }

            bool empty() const {
                return _stack.empty() && _cache.empty();
            }

            uint32_t size() const {
                return _stack.size() + _cache.size();
            }
        private:
            std::stack<ColoredPetriNetStateRandom> _stack;
            std::vector<ColoredPetriNetStateRandom> _cache;
            std::default_random_engine _rng;
        };
    }
}

#endif