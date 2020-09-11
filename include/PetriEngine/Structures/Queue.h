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
            Queue(StateSetInterface* states);
            virtual ~Queue();
            virtual bool pop(Structures::State& state) = 0;
            virtual void push(size_t id, PQL::DistanceContext&,
                std::shared_ptr<PQL::Condition>& query) = 0;
            size_t lastPopped()
            {
                return last;
            }
        protected:
            StateSetInterface* _states;
            size_t last = 0;
        };
        
        class BFSQueue : public Queue {
        public:
            BFSQueue(StateSetInterface* states);
            virtual ~BFSQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, PQL::DistanceContext&,
                std::shared_ptr<PQL::Condition>& query);
        private:
            size_t _cnt;
            size_t _nstates;
        };
        
        class DFSQueue : public Queue {
        public:
            DFSQueue(StateSetInterface* states);
            virtual ~DFSQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, PQL::DistanceContext&,
                std::shared_ptr<PQL::Condition>& query);
        private:
            std::stack<uint32_t> _stack;
        };
        
        class RDFSQueue : public Queue {
        public:
            RDFSQueue(StateSetInterface* states);
            virtual ~RDFSQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, PQL::DistanceContext&,
                std::shared_ptr<PQL::Condition>& query);
        private:
            std::stack<uint32_t> _stack;
            std::vector<uint32_t> _cache;
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

            HeuristicQueue(StateSetInterface* states);
            virtual ~HeuristicQueue();
            
            virtual bool pop(Structures::State& state);
            virtual void push(size_t id, PQL::DistanceContext&,
                std::shared_ptr<PQL::Condition>& query);
        private:
            std::priority_queue<weighted_t> _queue;
        };
    }
}

#endif /* QUEUE_H */

