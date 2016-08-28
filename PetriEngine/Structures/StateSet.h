/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef STATESET_H
#define STATESET_H
#include <unordered_map>
#include <iostream>
#include "Memory/pool_fixed_allocator.h"
#include "Memory/pool_dynamic_allocator.h"
#include "State.h"
#include "AlignedEncoder.h"
#include "ptrie.h"
#include "binarywrapper.h"


namespace PetriEngine {
    namespace Structures {
        
        class StateSetInterface
        {
        public:
            StateSetInterface(const PetriNet& net, uint32_t kbound) :
            _encoder(net.numberOfPlaces(), kbound), _net(net)
            {
                _discovered = 0;
                _kbound = kbound;
                _maxTokens = 0;
                _maxPlaceBound = std::vector<uint32_t>(_net.numberOfPlaces(), 0);
            }
            
            virtual std::pair<bool, size_t> add(State& state) = 0;
            
            virtual void decode(State& state, size_t id) = 0;
            
            virtual size_t size() const  = 0;
            
            const PetriNet& net() { return _net;}
            
            virtual void setHistory(size_t id, size_t transition) = 0;
            
            virtual std::pair<size_t, size_t> getHistory(size_t markingid) 
            { return std::make_pair(0,0); }
            
        protected:
            size_t _discovered;
            uint32_t _kbound;
            uint32_t _maxTokens;
            std::vector<uint32_t> _maxPlaceBound;
            AlignedEncoder _encoder;
            const PetriNet& _net;

#ifdef DEBUG
            std::vector<uint32_t*> _dbg;
#endif
            
            template<typename T>
            void _decode(State& state, size_t id, ptrie::ptrie_t<T>& _trie, T& _sp )
            {
                    typename ptrie::ptrie_t<T>::pointer_t ptr = 
                                            typename ptrie::ptrie_t<T>::pointer_t(&_trie, id);
                    uint32_t bits = ptr.write_partial_encoding(_sp);
                    for(size_t i = 0; i < bits; ++i)
                        _encoder.scratchpad().set(i, _sp.at((bits-1) - i));
                    
                    memcpy( &_encoder.scratchpad().raw()[bits/8],
                            ptr.remainder().const_raw(), 
                            ptr.remainder().size());

                    _encoder.decode(state.marking(), _encoder.scratchpad().raw());

#ifdef DEBUG
                    assert(memcmp(state.marking(), _dbg[id], sizeof(uint32_t)*_net.numberOfPlaces()) == 0);
#endif
            }             
            
            template<typename T>
            std::pair<bool, size_t> _add(State& state, ptrie::ptrie_t<T>& _trie, T& _sp) {
                _discovered++;

#ifdef DEBUG
                if(_discovered % 1000000 == 0) std::cout << "Found number " << _discovered << std::endl;
#endif
                
                MarkVal sum = 0;
                bool allsame = true;
                uint32_t val = 0;
                uint32_t active = 0;
                uint32_t last = 0;
                markingStats(state.marking(), sum, allsame, val, active, last);               

                if (_maxTokens < sum)
                    _maxTokens = sum;

                //Check that we're within k-bound
                if (_kbound != 0 && sum > _kbound)
                    return std::pair<bool, size_t>(false, std::numeric_limits<size_t>::max());
                    
                unsigned char type = _encoder.getType(sum, active, allsame, val);


                size_t length = _encoder.encode(state.marking(), type);
                T w = T(_encoder.scratchpad().raw(), length*8);
                auto tit = _trie.insert(w);
            
                
                if(!tit.first)
                {
                    return std::pair<bool, size_t>(false, std::numeric_limits<size_t>::max());
                }
                
#ifdef DEBUG
                _dbg.push_back(new uint32_t[_net.numberOfPlaces()]);
                memcpy(_dbg.back(), state.marking(), _net.numberOfPlaces()*sizeof(uint32_t));
                decode(state, _trie.size() - 1);
#endif
           
                // update the max token bound for each place in the net (only for newly discovered markings)
                for (uint32_t i = 0; i < _net.numberOfPlaces(); i++) 
                {
                    _maxPlaceBound[i] = std::max<MarkVal>( state.marking()[i],
                                                            _maxPlaceBound[i]);
                }

#ifdef DEBUG    
                if(_trie.size() % 100000 == 0) std::cout << "Inserted " << _trie.size() << std::endl;
#endif     
                return std::pair<bool, size_t>(true, _trie.size() - 1);
            }

        public:
            size_t discovered() const {
                return _discovered;
            }

            uint32_t maxTokens() const {
                return _maxTokens;
            }
            
            std::vector<MarkVal> maxPlaceBound() const {
                return _maxPlaceBound;
            }
            
        protected:
            
            void markingStats(const uint32_t* marking, MarkVal& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last)
            {
                uint32_t cnt = 0;
                
                for (uint32_t i = 0; i < _net.numberOfPlaces(); i++)
                {
                    uint32_t old = val;
                    if(marking[i] != 0)
                    {
                        ++cnt;
                        last = std::max(last, i);
                        val = std::max(marking[i], val);
                        if(old != 0 && marking[i] != old) allsame = false;
                        ++active;
                        sum += marking[i];
                    }
                }
            }
        };
        
#define STATESET_BUCKETS 1000000
        class StateSet : public StateSetInterface {
        private:
            using wrapper_t = ptrie::binarywrapper_t;
            using ptrie_t = ptrie::ptrie_t<wrapper_t>;
            
        public:

            StateSet(const PetriNet& net, uint32_t kbound) 
            : StateSetInterface(net, kbound) {
                _sp = wrapper_t(sizeof(uint32_t)*_net.numberOfPlaces()*8);
            }
            
            ~StateSet()
            {
                
            }
             
            virtual std::pair<bool, size_t> add(State& state) override
            {
                return _add<wrapper_t>(state, _trie, _sp);
            }
            
            virtual void decode(State& state, size_t id) override
            {
                _decode<wrapper_t>(state, id, _trie, _sp);
            }
            
            
            virtual size_t size() const override {
                return _trie.size();
            }

            virtual void setHistory(size_t id, size_t transition) override {}

        private:
            wrapper_t _sp;                    
            ptrie_t _trie;
        };

        class TracableStateSet : public StateSetInterface
        {
        public:
            struct traceable_t : public ptrie::binarywrapper_t
            {
                using binarywrapper_t::binarywrapper_t; // inherit all constructors
                ptrie::uint parent;
                ptrie::uint transition;
            } __attribute__((packed));
            
        private:

            using wrapper_t = traceable_t;
            using ptrie_t = ptrie::ptrie_t<wrapper_t>;
            

        public:

            TracableStateSet(const PetriNet& net, uint32_t kbound) 
            : StateSetInterface(net, kbound) {
                _sp = wrapper_t(sizeof(uint32_t)*_net.numberOfPlaces()*8);
            }
            
            ~TracableStateSet()
            {
                
            }
             
            virtual std::pair<bool, size_t> add(State& state) override
            {
#ifdef DEBUG
                size_t tmppar = _parent; // we might change this while debugging.
#endif
                auto res = _add<wrapper_t>(state, _trie, _sp);
#ifdef DEBUG
                _parent = tmppar;
#endif
                return res;
            }
            
            virtual void decode(State& state, size_t id) override
            {
                _parent = id;
                _decode<wrapper_t>(state, id, _trie, _sp);
            }
            
            
            virtual size_t size() const override {
                return _trie.size();
            }
            
            virtual void setHistory(size_t id, size_t transition) override 
            {
                ptrie_t::pointer_t ptr = ptrie_t::pointer_t(&_trie, id);
                ptr.remainder().parent = _parent;
                ptr.remainder().transition = transition;
            }
            
            virtual std::pair<size_t, size_t> getHistory(size_t markingid) override
            {
                ptrie_t::pointer_t ptr = ptrie_t::pointer_t(&_trie, markingid);
                size_t p = ptr.remainder().parent;
                size_t t = ptr.remainder().transition;
                return std::pair<size_t, size_t>(p, t);
            }
            
        private:
            wrapper_t _sp;                    
            ptrie_t _trie;
            size_t _parent = 0;
        };
        
    }
}


#endif // STATESET_H
