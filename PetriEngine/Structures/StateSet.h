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

            using ptrie_t = ptrie::ptrie_t<ptrie::binarywrapper_t<unsigned char>,unsigned char>;
            using wrapper_t = ptrie::binarywrapper_t<unsigned char>;

        public:

            StateSet(const PetriNet& net, uint32_t kbound, size_t memorylimit) 
            : _encoder(net.numberOfPlaces(), kbound) {
                _discovered = 0;
                _kbound = kbound;
                _maxTokens = 0;
                _places = net.numberOfPlaces();
                _maxPlaceBound = std::vector<uint32_t>(_places, 0);
                for(size_t i = 0; i < 32; ++i)
                {
                    _encoderstats[i] = 0;
                }
                _sp = wrapper_t(sizeof(uint32_t)*_places*8);
            }
            
            ~StateSet()
            {

                std::cout << "Encoding stats : " << std::endl;
                
                for(size_t i = 0; i < 32; ++i)
                {
                    std::cout << "\t type : " << i << " -> " << _encoderstats[i] << std::endl;
                }
            }
                       
            bool nextWaiting(State* state)
            {
                if(_waiting < _trie.size())
                {
                    ptrie_t::pointer_t ptr = ptrie_t::pointer_t(&_trie, _waiting);
                    uint32_t bits = ptr.write_partial_encoding(_sp);
                    uint32_t bytes = bits/ 8;
                    for(size_t i = 0; i < bits; ++i)
                        _encoder.scratchpad().set(i, _sp.at((bits-1) - i));
                    
                    memcpy( &_encoder.scratchpad().raw()[bits/8],
                            ptr.remainder().const_raw(), 
                            ptr.remainder().size());
                    
                    _encoder.decode(state->marking(), _encoder.scratchpad().raw(), ptr.get_meta());

                    ++_waiting;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            
            bool add(State* state) {
                _discovered++;

#ifdef DEBUG
                if(_discovered % 1000000 == 0) std::cout << "Found number " << _discovered << std::endl;
#endif
                
                MarkVal sum = 0;
                bool allsame = true;
                uint32_t val = 0;
                uint32_t active = 0;
                uint32_t last = 0;
                uint32_t hash = hashMarking(state->marking(), sum, allsame, val, active, last);               

                if (_maxTokens < sum)
                    _maxTokens = sum;

                //Check that we're within k-bound
                if (_kbound != 0 && sum > _kbound)
                    return false;
    
                uint32_t nfree;

                
                unsigned char type = _encoder.getType(sum, active, allsame, val);
                type += 32* last;

                size_t size;
                const unsigned char* tomatch;
                size_t length = _encoder.encode(state->marking(), type);
                _encoder.scratchpad().raw()[length] = type;
                ++ length;
                wrapper_t w = wrapper_t(_encoder.scratchpad().raw(), length*8);
                w.set_meta(type);
                auto tit = _trie.insert(w);
            
                
                if(!tit.first)
                {
                    return false;
                }

                _encoderstats[type & 31] += 1;
           
                // update the max token bound for each place in the net (only for newly discovered markings)
                for (uint32_t i = 0; i < _places; i++) 
                {
                    _maxPlaceBound[i] = std::max<MarkVal>( state->marking()[i],
                                                            _maxPlaceBound[i]);
                }
                
                if(_trie.size() % 100000 == 0) std::cout << "Inserted " << _trie.size() << std::endl;
                
                return true;
            }

            size_t discovered() const {
                return _discovered;
            }

            uint32_t maxTokens() const {
                return _maxTokens;
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
                
                for (uint32_t i = 0; i < _places; i++)
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
            uint32_t _places;

            size_t _waiting = 0;
            AlignedEncoder _encoder;
                    
            size_t _encoderstats[32];
            
            ptrie_t _trie;
            wrapper_t _sp;
        };

    }
}


#endif // STATESET_H
