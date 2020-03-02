/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/PQL/Contexts.h"

#include <algorithm>
#include <random>

namespace PetriEngine {
    namespace Structures {
        Queue::Queue(StateSetInterface* states) : _states(states) {} 

        Queue::~Queue() {
        }
        
        
        BFSQueue::BFSQueue(StateSetInterface* states) : Queue(states), _cnt(0), _nstates(0) {}
        BFSQueue::~BFSQueue(){}
                
        bool BFSQueue::pop(Structures::State& state)
        {
            if(_cnt < _nstates)
            {
                _states->decode(state, _cnt);
                ++_cnt;
                return true;
            }
            else
            {
                return false;
            }
        }
        
        void BFSQueue::push(size_t id, PQL::DistanceContext&,
            std::shared_ptr<PQL::Condition>& query)
        {
            ++_nstates;
            // nothing
        }
        
        DFSQueue::DFSQueue(StateSetInterface* states) : Queue(states) {}
        DFSQueue::~DFSQueue(){}
                
        bool DFSQueue::pop(Structures::State& state)
        {
            if(_stack.empty()) return false;
            uint32_t n = _stack.top();
            _stack.pop();
            _states->decode(state, n);
            return true;
        }
        
        void DFSQueue::push(size_t id, PQL::DistanceContext&,
            std::shared_ptr<PQL::Condition>& query)
        {
            _stack.push(id);
        }
        
        RDFSQueue::RDFSQueue(StateSetInterface* states) : Queue(states) {}
        RDFSQueue::~RDFSQueue(){}
       
        auto rng = std::default_random_engine {};
        bool RDFSQueue::pop(Structures::State& state)
        {
            if(_cache.empty())
            {
                if(_stack.empty()) return false;
                uint32_t n = _stack.top();
                _stack.pop();
                _states->decode(state, n);
                return true;                
            }
            else
            {
                std::shuffle ( _cache.begin(), _cache.end(), rng );
		uint32_t n = _cache.back();
                _states->decode(state, n);
                for(size_t i = 0; i < (_cache.size() - 1); ++i)
                {
                    _stack.push(_cache[i]);
                }
                _cache.clear();
                return true;
            }
        }
        
        void RDFSQueue::push(size_t id, PQL::DistanceContext&,
            std::shared_ptr<PQL::Condition>& query)
        {
            _cache.push_back(id);
        }
        
        HeuristicQueue::HeuristicQueue(StateSetInterface* states) : Queue(states) {}
        HeuristicQueue::~HeuristicQueue(){}
                
        bool HeuristicQueue::pop(Structures::State& state)
        {
            if(_queue.empty()) return false;
            uint32_t n = _queue.top().item;
            _queue.pop();
            _states->decode(state, n);
            return true;
        }
        
        void HeuristicQueue::push(size_t id, PQL::DistanceContext& context,
            std::shared_ptr<PQL::Condition>& query)
        {
            // invert result, highest numbers are on top!
            uint32_t dist = query->distance(context);
            _queue.emplace(dist, (uint32_t)id);
        }

    }
}
