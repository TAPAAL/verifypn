#ifndef POTENCY_QUEUE_H
#define POTENCY_QUEUE_H

#include <queue>

#include "../PQL/PQL.h"

namespace PetriEngine {
    namespace Structures {
        class PotencyQueue {
        public:
            struct weighted_t {
                uint32_t weight;
                size_t item;

                weighted_t(uint32_t w, size_t i) : weight(w), item(i) {};

                bool operator<(const weighted_t &y) const {
                    if (weight == y.weight)
                        return item < y.item;
                    return weight > y.weight;
                }
            };

            PotencyQueue(size_t seed = 0);
            PotencyQueue(const std::vector<MarkVal> &initPotencies);
            PotencyQueue(const std::vector<MarkVal> &initPotencies, size_t seed);

            virtual ~PotencyQueue();

            size_t pop();

            bool empty() const;

            void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query);

            virtual void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) = 0;

        protected:
            size_t _size = 0;
            size_t _best;
            uint32_t _currentParentDist;
            std::vector<uint32_t> _potencies;
            std::vector<std::priority_queue<weighted_t>> _queues;

            const static uint32_t _initPotencyConstant = 60;

            void _initializePotencies(size_t nTransitions, uint32_t initValue);
            void _initializePotencies(const std::vector<MarkVal> &initPotencies);
        };

        class RandomPotencyQueue : public PotencyQueue {
        public:
            RandomPotencyQueue() = default;
            RandomPotencyQueue(size_t seed);
            RandomPotencyQueue(const std::vector<MarkVal> &initPotencies, size_t seed);

            virtual ~RandomPotencyQueue();

            using PotencyQueue::push;

            void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) override;

            size_t pop();

        private:
            size_t _seed;
        };
    }
}

#endif /* POTENCY_QUEUE_H */
