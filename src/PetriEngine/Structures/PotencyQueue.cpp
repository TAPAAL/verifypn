#include "PetriEngine/Structures/PotencyQueue.h"
#include "PetriEngine/PQL/Contexts.h"

namespace PetriEngine {
    namespace Structures {
        PotencyQueue::PotencyQueue(const std::vector<MarkVal> &initPotencies) {
            _initializePotencies(initPotencies);
        }

        PotencyQueue::PotencyQueue(const std::vector<MarkVal> &initPotencies, size_t seed) : PotencyQueue(initPotencies) {}

        PotencyQueue::PotencyQueue(size_t seed) {}

        PotencyQueue::~PotencyQueue() {}

        size_t PotencyQueue::pop() {
            if (_size == 0)
                return PetriEngine::PQL::EMPTY;

            size_t t = _best;
            while (_queues[t].empty()) {
                ++t;
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

        void PotencyQueue::_initializePotencies(size_t nTransitions, uint32_t initValue) {
            _queues = std::vector<std::priority_queue<weighted_t>>(nTransitions != 0 ? nTransitions : 1);

            _potencies.reserve(nTransitions);
            for (uint32_t i = 0; i < nTransitions; i++) {
                _potencies.push_back(initValue);
            }
            _best = 0;
        }

        void PotencyQueue::_initializePotencies(const std::vector<MarkVal> &initPotencies) {
            _queues = std::vector<std::priority_queue<weighted_t>>(initPotencies.size() != 0 ? initPotencies.size() : 1);

            _potencies.reserve(initPotencies.size());
            uint32_t maxiInitPotencies = *std::max_element(initPotencies.begin(), initPotencies.end());
            for (auto potency : initPotencies) {
                _potencies.push_back(potency * _initPotencyMultiplier / maxiInitPotencies + _initPotencyConstant);
            }
            _best = 0;
        }

        RandomPotencyQueue::RandomPotencyQueue(size_t seed) : PotencyQueue(seed), _seed(seed) {
            srand(_seed);
        }

        RandomPotencyQueue::RandomPotencyQueue(const std::vector<MarkVal> &initPotencies, size_t seed) : PotencyQueue(initPotencies, seed), _seed(seed) {
            srand(_seed);
        }

        RandomPotencyQueue::~RandomPotencyQueue() {}

        void
        RandomPotencyQueue::push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) {
            uint32_t dist = query->distance(*context);

            if (dist < _currentParentDist) {
                _potencies[t] += _currentParentDist - dist;
            } else if (dist > _currentParentDist && _potencies[t] != 0) {
                if (_potencies[t] - 1 >= dist - _currentParentDist)
                    _potencies[t] -= dist - _currentParentDist;
                else
                    _potencies[t] = 1;
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

            for (size_t t = 0; t < _potencies.size(); ++t) {
                if (_queues[t].empty()) {
                    continue;
                }

                n += _potencies[t];
                double r = (double) rand() / RAND_MAX;
                double threshold = _potencies[t] / (double) n;
                if (r <= threshold)
                    current = t;
            }

            weighted_t e = _queues[current].top();
            _queues[current].pop();
            _size--;
            _currentParentDist = e.weight;
            return e.item;
        }
    }
}
