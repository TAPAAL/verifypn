/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Queue.h
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 21:12
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <memory>
#include <queue>
#include <stack>
#include <random>

#include "../PQL/PQL.h"

namespace PetriEngine {
    namespace Structures {
        class Queue {
        public:
            Queue(size_t s = 0);
            virtual ~Queue();
            virtual size_t pop() = 0;

            virtual void push(size_t id, PQL::DistanceContext* = nullptr,
                const PQL::Condition* query = nullptr) = 0;
            virtual bool empty() const = 0;
            static constexpr size_t EMPTY = std::numeric_limits<size_t>::max();
        };

        class BFSQueue : public Queue {
        public:
            BFSQueue(size_t);
            virtual ~BFSQueue();

            virtual size_t pop() override;
            virtual void push(size_t id, PQL::DistanceContext*,
                const PQL::Condition* query) override;
            virtual bool empty() const override;
        private:
            std::queue<uint32_t> _queue;
            std::vector<uint32_t> _cache;
            std::default_random_engine _rng;
        };

        class DFSQueue : public Queue {
        public:
            DFSQueue(size_t);
            virtual ~DFSQueue();

            virtual size_t pop();
            //bool top(Structures::State& state) const;
            virtual void push(size_t id, PQL::DistanceContext*,
                const PQL::Condition* query);
            virtual bool empty() const override;
        private:
            std::stack<uint32_t> _stack;
        };

        class RDFSQueue : public Queue {
        public:
            RDFSQueue(size_t seed);
            virtual ~RDFSQueue();

            virtual size_t pop();
            virtual void push(size_t id, PQL::DistanceContext*,
                const PQL::Condition* query);
            virtual bool empty() const override;
        private:
            std::stack<uint32_t> _stack;
            std::vector<uint32_t> _cache;
            std::default_random_engine _rng;
        };

        class HeuristicQueue : public Queue {
        public:
            struct weighted_t {
                uint32_t weight;
                uint32_t item;
                weighted_t(uint32_t w, uint32_t i) : weight(w), item(i) {};
                bool operator <(const weighted_t& y) const {
                    if(weight == y.weight) return item < y.item;// do dfs if they match
//                    if(weight == y.weight) return item > y.item;// do bfs if they match
                    return weight > y.weight;
                }
            };

            HeuristicQueue(size_t);
            virtual ~HeuristicQueue();

            virtual size_t pop();
            virtual void push(size_t id, PQL::DistanceContext*,
                const PQL::Condition* query);
            virtual bool empty() const override;
        private:
            std::priority_queue<weighted_t> _queue;
        };
    }
}

#endif /* QUEUE_H */

