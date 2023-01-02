#include "PetriEngine/Structures/PotencyQueue.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    namespace Structures {
        PotencyQueue::PotencyQueue(size_t nTransitions, size_t s) : _queues(nTransitions) {
            if (nTransitions == 0)
                _queues = std::vector<std::priority_queue<weighted_t>>(1);

            _potencies.reserve(nTransitions);
            for (uint32_t i = 0; i < nTransitions; i++) {
                size_t prev = i == 0 ? SIZE_MAX : i - 1;
                size_t next = i == nTransitions - 1 ? SIZE_MAX : i + 1;
                _potencies.push_back(potency_t(100, prev, next));
            }
            _best = 0;
        }

        PotencyQueue::~PotencyQueue() {}

        std::tuple<size_t, uint32_t> PotencyQueue::pop() {
            if (_size == 0)
                return std::make_tuple(PetriEngine::PQL::EMPTY, PetriEngine::PQL::EMPTY);

            size_t t = _best;
            while (_queues[t].empty()) {
                t = _potencies[t].next;
            }
            weighted_t n = _queues[t].top();
            _queues[t].pop();
            _size--;
            return std::make_tuple(n.item, n.weight);
        }

        void PotencyQueue::push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query) {
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

        RandomPotencyQueue::RandomPotencyQueue(size_t nTransitions, size_t seed) : PotencyQueue(nTransitions),
                                                                                   _seed(seed) {
            srand(_seed);
        }

        RandomPotencyQueue::~RandomPotencyQueue() {}

        void RandomPotencyQueue::push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t,
                                      uint32_t pDist) {
        }

        std::tuple<size_t, uint32_t> RandomPotencyQueue::pop() {
            return ;
        }
    }
}
