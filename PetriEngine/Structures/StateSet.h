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
#define STATESET_BUCKETS 1000000
        class StateSet {
        private:

            using wrapper_t = ptrie::binarywrapper_t;
            using ptrie_t = ptrie::ptrie_t<wrapper_t>;
            

        public:

            StateSet(const PetriNet& net, uint32_t kbound) 
            : _encoder(net.numberOfPlaces(), kbound), _net(net) {
                _discovered = 0;
                _kbound = kbound;
                _maxTokens = 0;
                _maxPlaceBound = std::vector<uint32_t>(_net.numberOfPlaces(), 0);
                _sp = wrapper_t(sizeof(uint32_t)*_net.numberOfPlaces()*8);
            }
            
            ~StateSet()
            {

            }
            
            const PetriNet& net() { return _net;}
             
            void decode(State& state, size_t id )
            {
                    ptrie_t::pointer_t ptr = ptrie_t::pointer_t(&_trie, id);
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
            
            std::pair<bool, size_t> add(State& state) {
                _discovered++;

#ifdef DEBUG
                if(_discovered % 1000000 == 0) std::cout << "Found number " << _discovered << std::endl;
#endif
                
                MarkVal sum = 0;
                bool allsame = true;
                uint32_t val = 0;
                uint32_t active = 0;
                uint32_t last = 0;
                uint32_t hash = hashMarking(state.marking(), sum, allsame, val, active, last);               

                if (_maxTokens < sum)
                    _maxTokens = sum;

                //Check that we're within k-bound
                if (_kbound != 0 && sum > _kbound)
                    return std::pair<bool, size_t>(false, std::numeric_limits<size_t>::max());
                    
                unsigned char type = _encoder.getType(sum, active, allsame, val);


                size_t length = _encoder.encode(state.marking(), type);

                wrapper_t w = wrapper_t(_encoder.scratchpad().raw(), length*8);
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

            size_t discovered() const {
                return _discovered;
            }

            uint32_t maxTokens() const {
                return _maxTokens;
            }
            
            size_t size() const {
                return _trie.size();
            }

            std::vector<MarkVal> maxPlaceBound() const {
                return _maxPlaceBound;
            }

        private:
            
            uint32_t hashMarking(const uint32_t* marking, MarkVal& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last)
            {
                uint32_t hash = 0;
                
                 uint16_t& h1 = ((uint16_t*)&hash)[0];
                uint16_t& h2 = ((uint16_t*)&hash)[1];
                uint32_t cnt = 0;
                
                for (uint32_t i = 0; i < _net.numberOfPlaces(); i++)
                {
                    uint32_t old = val;
                    if(marking[i] != 0)
                    {
                        if(allsame)
                        {
                            hash ^= (1 << (i % 32));
                        }
                        else
                        {
                            h1 ^= (1 << (i % 16));
                            h2 ^= (marking[i] << (cnt % 16));
                        }
                        ++cnt;
                        last = std::max(last, i);
                        val = std::max(marking[i], val);
                        if(old != 0 && marking[i] != old) allsame = false;
                        ++active;
                        sum += marking[i];
                    }
                }
                return hash;
            }
            
            size_t _discovered;
            uint32_t _kbound;
            uint32_t _maxTokens;
            std::vector<uint32_t> _maxPlaceBound;

            AlignedEncoder _encoder;
                    
            ptrie_t _trie;
            wrapper_t _sp;
            const PetriNet& _net;
#ifdef DEBUG
            std::vector<uint32_t*> _dbg;
#endif
        };

    }
}


#endif // STATESET_H
