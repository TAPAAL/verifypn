#ifndef COLORED_SEARCH_TYPES_H
#define COLORED_SEARCH_TYPES_H

#include <queue>
#include "PetriEngine/ExplicitColored/Visitors/HeuristicVisitor.h"

namespace PetriEngine {
    namespace ExplicitColored {
        template <typename T>
        class DFSStructure {
        public:
            T& next() {
                return waiting.top();
            }

            void remove() {
                waiting.pop();
            }

            void add(T state) {
                waiting.emplace(std::move(state));
            }

            bool empty() const {
                return waiting.empty();
            }

            uint32_t size() const {
                return waiting.size();
            }
        private:
            std::stack<T> waiting;
        };

        template <typename T>
        class BFSStructure {
        public:
            T& next() {
                return waiting.front();
            }

            void remove() {
                waiting.pop();
            }

            void add(T state) {
                waiting.emplace(std::move(state));
            }

            bool empty() const {
                return waiting.empty();
            }

            uint32_t size() const {
                return waiting.size();
            }
        private:
            std::queue<T> waiting;
        };

        template<typename T>
        class RDFSStructure {
        public:
            explicit RDFSStructure(const size_t seed) : _rng(seed) {}
            T& next() {
                if (_stack.empty()) {
                    shuffle();
                }
                return _stack.top();
            }

            void remove() {
                _stack.pop();
                shuffle();
            }

            void shuffle() {
                std::shuffle(_cache.begin(), _cache.end(), _rng);
                for (auto & it : _cache) {
                    _stack.emplace(std::move(it));
                }
                _cache.clear();
            }

            void add(T state) {
                _cache.push_back(std::move(state));
            }

            bool empty() const {
                return _stack.empty() && _cache.empty();
            }

            uint32_t size() const {
                return _stack.size() + _cache.size();
            }
        private:
            std::stack<T> _stack;
            std::vector<T> _cache;
            std::default_random_engine _rng;
        };

        struct WeightedState {
            mutable ColoredPetriNetState cpn;
            MarkingCount_t weight;
            bool operator<(const WeightedState& other) const {
                return weight < other.weight;
            }
        };

        template <typename T>
        class BestFSStructure {
        public:
            explicit BestFSStructure(
                const size_t seed, PQL::Condition_ptr query, const std::unordered_map<std::string,
                uint32_t>& placeNameIndices, const bool negQuery
                )
                : _rng(seed), _query(std::move(query)), _negQuery(negQuery), _placeNameIndices(placeNameIndices) {}

            T& next() const {
                return _queue.top().cpn;
            }

            void remove() {
                _queue.pop();
            }

            void add(T state) {
                const MarkingCount_t weight = DistQueryVisitor::eval(_query, state.marking, _placeNameIndices, _negQuery);
                if (weight <_minDist) {
                    _minDist = weight; //Why
                }
                _queue.push(WeightedState {
                    std::move(state),
                    weight
                });
            }

            bool empty() const {
                return _queue.empty();
            }

            uint32_t size() const {
                return _queue.size();
            }
        private:
            std::priority_queue<WeightedState> _queue;
            std::default_random_engine _rng;
            PQL::Condition_ptr _query;
            bool _negQuery;
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
            MarkingCount_t _minDist = std::numeric_limits<MarkingCount_t>::max();
        };
    }
}

#endif