#ifndef POTENCY_QUEUE_H
#define POTENCY_QUEUE_H

#include <queue>

#include "../PQL/PQL.h"

namespace PetriEngine
{
    namespace Structures
    {
        class PotencyQueue
        {
        public:
            struct weighted_t
            {
                uint32_t weight;
                size_t item;

                weighted_t(uint32_t w, size_t i) : weight(w), item(i){};

                bool operator<(const weighted_t &y) const
                {
                    if (weight == y.weight)
                        return item < y.item;
                    return weight > y.weight;
                }
            };

            struct potency_t
            {
                uint32_t value;
                size_t prev;
                size_t next;

                potency_t(uint32_t v, size_t p, size_t n) : value(v), prev(p), next(n){};
            };

            PotencyQueue(size_t nTransitions, size_t s = 0);

            virtual ~PotencyQueue();

            std::tuple<size_t, uint32_t> pop();

            bool empty() const;

            void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query);

            virtual void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t,
                              uint32_t pDist) = 0;

        protected:
            size_t _size = 0;
            size_t _best;
            std::vector<potency_t> _potencies;
            std::vector<std::priority_queue<weighted_t>> _queues;

            void _swapAdjacent(size_t a, size_t b);
        };

        class RandomPotencyQueue : public PotencyQueue
        {
        public:
            RandomPotencyQueue(size_t nTransitions, size_t seed);

            virtual ~RandomPotencyQueue();

            using PotencyQueue::push;

            virtual void push(size_t id, PQL::DistanceContext *context, const PQL::Condition *query, uint32_t t,
                              uint32_t pDist) override;

            std::tuple<size_t, uint32_t> pop();

        private:
            size_t _seed;
        };
    }
}

#endif /* POTENCY_QUEUE_H */
