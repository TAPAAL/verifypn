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

            struct potency_t {
                uint32_t value;
                size_t prev;
                size_t next;

                potency_t(uint32_t v, size_t p, size_t n) : value(v), prev(p), next(n) {};
            };

            PotencyQueue(size_t s = 0);

            virtual ~PotencyQueue();

            size_t pop();

            bool empty() const;

            void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query);

            virtual void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) = 0;

        protected:
            size_t _size = 0;
            size_t _best;
            uint32_t _currentParentDist;
            std::vector<potency_t> _potencies;
            std::vector<std::priority_queue<weighted_t>> _queues;

            void _swapAdjacent(size_t a, size_t b);

            void _initializePotencies(size_t nTransitions, uint32_t initValue);
        };

        class RandomPotencyQueue : public PotencyQueue {
        public:
            RandomPotencyQueue(size_t seed);

            virtual ~RandomPotencyQueue();

            using PotencyQueue::push;

            void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t) override;

            size_t pop();

        protected:
            size_t _seed;
        };

        class RandomWalkPotencyQueue : public RandomPotencyQueue {
        public:
            RandomWalkPotencyQueue(size_t seed);

            virtual ~RandomWalkPotencyQueue();

            /* Returns the next element according to the potencies.
             * Then calls resetQueue() to reset the queue.
             */
            size_t pop();

            /* Resets the queue to the initial state.
             * The initial state is the state where the queue contains empty priority_queues.
             * Uses the actual size of _queue to determine the number of empty priority_queues.
             * Also sets _size to 0.
             */
            void resetQueue();
        };
    }
}

#endif /* POTENCY_QUEUE_H */
