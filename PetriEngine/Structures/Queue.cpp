/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Queue.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 30 March 2016, 21:12
 */

#include "Queue.h"
#include "../PQL/Contexts.h"

namespace PetriEngine {
    namespace Structures {
        Queue::Queue(StateSet& states) : _states(states) {} 

        Queue::~Queue() {
        }
        
        
        BFSQueue::BFSQueue(StateSet& states) : Queue(states), _cnt(0) {}
        BFSQueue::~BFSQueue(){}
                
        bool BFSQueue::pop(Structures::State& state)
        {
            if(_cnt < _states.size())
            {
                _states.decode(state, _cnt);
                ++_cnt;
                return true;
            }
            else
            {
                return false;
            }
        }
        
        void BFSQueue::push(size_t id, Structures::State& state,
            std::shared_ptr<PQL::Condition>& query)
        {
            // nothing
        }
        
        DFSQueue::DFSQueue(StateSet& states) : Queue(states) {}
        DFSQueue::~DFSQueue(){}
                
        bool DFSQueue::pop(Structures::State& state)
        {
            if(_stack.empty()) return false;
            uint32_t n = _stack.top();
            _stack.pop();
            _states.decode(state, n);
            return true;
        }
        
        void DFSQueue::push(size_t id, Structures::State& state,
            std::shared_ptr<PQL::Condition>& query)
        {
            _stack.push(id);
        }
        
        HeuristicQueue::HeuristicQueue(StateSet& states) : Queue(states) {}
        HeuristicQueue::~HeuristicQueue(){}
                
        bool HeuristicQueue::pop(Structures::State& state)
        {
            if(_queue.empty()) return false;
            uint32_t n = _queue.top().item;
            _queue.pop();
            _states.decode(state, n);
            return true;
        }
        
        void HeuristicQueue::push(size_t id, Structures::State& state,
            std::shared_ptr<PQL::Condition>& query)
        {
            PQL::DistanceContext context(&_states.net(), state.marking());
            // invert result, highest numbers are on top!
            uint32_t dist = std::numeric_limits<uint32_t>::max() - query->distance(context);
            _queue.emplace(dist, (uint32_t)id);
        }

    }
}
