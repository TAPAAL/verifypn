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

#include "../PQL/PQL.h"
#include "StateSet.h"

namespace PetriEngine {
    namespace Structures {
        class Queue {
        public:
            Queue(StateSet& states);
            virtual ~Queue();
            virtual bool pop(Structures::State& state) = 0;
            virtual void push(size_t id, Structures::State& state,
                std::shared_ptr<PQL::Condition>& query) = 0;
        protected:
            StateSet& _states;
        };
        
        class BFSQueue : public Queue {
        public:
            BFSQueue(StateSet& states);
            virtual ~BFSQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, Structures::State& state,
                std::shared_ptr<PQL::Condition>& query);
        private:
            size_t _cnt;
        };
        
        class DFSQueue : public Queue {
        public:
            DFSQueue(StateSet& states);
            virtual ~DFSQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, Structures::State& state,
                std::shared_ptr<PQL::Condition>& query);
        private:
            std::stack<uint32_t> _stack;
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
                    return weight < y.weight;
                }
            };

            HeuristicQueue(StateSet& states);
            virtual ~HeuristicQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, Structures::State& state,
                std::shared_ptr<PQL::Condition>& query);
        private:
            std::priority_queue<weighted_t> _queue;
        };
    }
}

#endif /* QUEUE_H */

