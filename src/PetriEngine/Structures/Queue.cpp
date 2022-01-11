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
        Queue::Queue(size_t) {}

        Queue::~Queue() {
        }


        BFSQueue::BFSQueue(size_t) : Queue() {}
        BFSQueue::~BFSQueue(){}

        size_t BFSQueue::pop()
        {
            if(!_queue.empty())
            {
                auto r = _queue.front();
                _queue.pop();
                return r;
            }
            else
            {
                return EMPTY;
            }
        }

        void BFSQueue::push(size_t id, PQL::DistanceContext*,
            const PQL::Condition* query)
        {
            _queue.push(id);
        }

        bool BFSQueue::empty() const {
            return _queue.empty();
        }

        DFSQueue::DFSQueue(size_t) : Queue() {}
        DFSQueue::~DFSQueue(){}

        size_t DFSQueue::pop()
        {
            if(_stack.empty()) return EMPTY;
            uint32_t n = _stack.top();
            _stack.pop();
            return n;
        }

        void DFSQueue::push(size_t id, PQL::DistanceContext*,
            const PQL::Condition* query)
        {
            _stack.push(id);
        }

        bool DFSQueue::empty() const {
            return _stack.empty();
        }

        /*bool DFSQueue::top() const {
            if(_stack.empty()) return EMPTY;
            uint32_t n = _stack.top();
            _states->decode(state, n);
            return true;
        }*/

        RDFSQueue::RDFSQueue(size_t seed) : Queue()
        {
            _rng.seed(seed);
        }

        RDFSQueue::~RDFSQueue(){}

        size_t RDFSQueue::pop()
        {
            if(_cache.empty())
            {
                if(_stack.empty())
                    return EMPTY;
                uint32_t n = _stack.top();
                _stack.pop();
                return n;
            }
            else
            {
                std::shuffle(_cache.begin(), _cache.end(), _rng);
                uint32_t n = _cache.back();
                for(size_t i = 0; i < (_cache.size() - 1); ++i)
                {
                    _stack.push(_cache[i]);
                }
                _cache.clear();
                return n;
            }
        }

        /*bool RDFSQueue::top(State &state) {
            if (!_cache.empty()) {
                std::shuffle ( _cache.begin(), _cache.end(), _rng );
                uint32_t n = _cache.back();
                _states->decode(state, n);
                for(size_t i = 0; i < _cache.size(); ++i)
                {
                    _stack.push(_cache[i]);
                }
                _cache.clear();
            }
            if (_stack.empty()) return EMPTY;
            uint32_t n = _stack.top();
            return n;
        }*/

        void RDFSQueue::push(size_t id, PQL::DistanceContext*,
            const PQL::Condition* query)
        {
            _cache.push_back(id);
        }

        bool RDFSQueue::empty() const {
            return _cache.empty() && _stack.empty();
        }

        HeuristicQueue::HeuristicQueue(size_t) : Queue() {}
        HeuristicQueue::~HeuristicQueue(){}

        size_t HeuristicQueue::pop()
        {
            if(_queue.empty()) return EMPTY;
            uint32_t n = _queue.top().item;
            _queue.pop();
            return n;
        }

        void HeuristicQueue::push(size_t id, PQL::DistanceContext* context,
            const PQL::Condition* query)
        {
            // invert result, highest numbers are on top!
            uint32_t dist = query->distance(*context);
            _queue.emplace(dist, (uint32_t)id);
        }

        bool HeuristicQueue::empty() const {
            return _queue.empty();
        }

    }
}
