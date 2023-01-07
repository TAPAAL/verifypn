#include "PetriEngine/Structures/PotencyQueue.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    namespace Structures {
        PotencyQueue::PotencyQueue(size_t s) {}

        PotencyQueue::~PotencyQueue() {}

        size_t PotencyQueue::pop() {
            if (_size == 0)
                return PetriEngine::PQL::EMPTY;

            size_t t = _best;
            while (_queues[t].empty()) {
                t = _potencies[t].next;
            }
            weighted_t n = _queues[t].top();
            _queues[t].pop();
            _size--;
            _currentParentDist = n.weight;
            return n.item;
        }

        void PotencyQueue::push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query) {
            if (_potencies.empty())
                this->_initializePotencies(context->net()->numberOfTransitions(), 100);

            uint32_t dist = query->distance(*context);
            _queues[_best].emplace(dist, id);
            _size++;
        }

        bool PotencyQueue::empty() const {
            return _size == 0;
        }

        void PotencyQueue::_swapAdjacent(size_t a, size_t b) {
            // x <-> a <-> b <-> y
            // Assert: _potencies[a].next == b && _potencies[b].prev == a

            // x
            if (_potencies[a].prev != SIZE_MAX)
                _potencies[_potencies[a].prev].next = b;

            // y
            if (_potencies[b].next != SIZE_MAX)
                _potencies[_potencies[b].next].prev = a;

            // a
            size_t prevTmp = _potencies[a].prev;
            _potencies[a].prev = b;
            _potencies[a].next = _potencies[b].next;

            // b
            _potencies[b].prev = prevTmp;
            _potencies[b].next = a;
        }

        void PotencyQueue::_initializePotencies(size_t nTransitions, uint32_t initValue) {
            if (nTransitions == 0)
                _queues = std::vector<std::priority_queue<weighted_t>>(1);

            _potencies.reserve(nTransitions);
            for (uint32_t i = 0; i < nTransitions; i++) {
                size_t prev = i == 0 ? SIZE_MAX : i - 1;
                size_t next = i == nTransitions - 1 ? SIZE_MAX : i + 1;
                _potencies.push_back(potency_t(initValue, prev, next));
            }
            _best = 0;
        }

        RandomPotencyQueue::RandomPotencyQueue(size_t seed) : PotencyQueue(seed), _seed(seed) {
            srand(_seed);
        }

        RandomPotencyQueue::~RandomPotencyQueue() {}

        void
        RandomPotencyQueue::push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) {
            uint32_t dist = query->distance(*context);

            if (dist < _currentParentDist) {
                _potencies[t].value += _currentParentDist - dist;
                while (_potencies[t].prev != SIZE_MAX && _potencies[t].value > _potencies[_potencies[t].prev].value) {
                    _swapAdjacent(_potencies[t].prev, t);
                }

                if (_potencies[t].prev == SIZE_MAX)
                    _best = t;
            } else if (dist > _currentParentDist && _potencies[t].value != 0) {
                if (_potencies[t].value - 1 >= dist - _currentParentDist)
                    _potencies[t].value -= dist - _currentParentDist;
                else
                    _potencies[t].value = 1;
                while (_potencies[t].next != SIZE_MAX && _potencies[t].value < _potencies[_potencies[t].next].value) {
                    if (_best == t)
                        _best = _potencies[t].next;

                    _swapAdjacent(t, _potencies[t].next);
                }
            }

            _queues[t].emplace(dist, id);
            _size++;
        }

        size_t RandomPotencyQueue::pop() {
            if (_size == 0)
                return PetriEngine::PQL::EMPTY;

            if (_potencies.empty()) {
                weighted_t e = _queues[_best].top();
                _queues[_best].pop();
                _size--;
                _currentParentDist = e.weight;
                return e.item;
            }

            uint32_t n = 0;
            size_t current = SIZE_MAX;

            size_t t = _best;
            while (t != SIZE_MAX) {
                if (_queues[t].empty()) {
                    t = _potencies[t].next;
                    continue;
                }

                n += _potencies[t].value;
                float r = (float) rand() / RAND_MAX;
                float threshold = _potencies[t].value / (float) n;
                if (r <= threshold)
                    current = t;

                t = _potencies[t].next;
            }

            weighted_t e = _queues[current].top();
            _queues[current].pop();
            _size--;
            _currentParentDist = e.weight;
            return e.item;
        }
    }
}
