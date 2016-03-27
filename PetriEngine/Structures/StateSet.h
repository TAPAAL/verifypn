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

namespace PetriEngine {
    namespace Structures {
#define STATESET_BUCKETS 1000000
        class StateSet {
        private:
            
            struct bucket_t
            {
                unsigned char* data;
                uint32_t next;
                unsigned char type;
#ifdef DEBUG
                uint32_t* org;
#endif
            } __attribute__((packed));
                        
            using mapnode_t = std::pair<uint32_t, uint32_t>;
            using mapallocator_t = palloc::pool_fixed_allocator<mapnode_t>;
            using dataallocator_t = pdalloc::pool_dynamic_allocator<unsigned char>;


        public:

            StateSet(const PetriNet& net, uint32_t kbound, size_t memorylimit) 
            : _encoder(net.numberOfPlaces(), kbound) {
                _discovered = 0;
                _kbound = kbound;
                _maxTokens = 0;
                _places = net.numberOfPlaces();
                _maxPlaceBound = std::vector<uint32_t>(_places, 0);
                _map.reserve(1024*1024);
                for(size_t i = 0; i < 32; ++i)
                {
                    _encoderstats[i] = 0;
                }
                _memorylimit = memorylimit;
                _encoding = _memorylimit == 0;
            }
            
            ~StateSet()
            {
                for(auto d : _buckets)
                {
                    delete[] d;
                }
                std::cout << "Encoding stats : " << std::endl;
                
                for(size_t i = 0; i < 32; ++i)
                {
                    std::cout << "\t type : " << i << " -> " << _encoderstats[i] << std::endl;
                }
                std::cout << "collisions : " << collisions << std::endl << " of " << (_freebucket-1) << std::endl;
            }
                        
            void decompress(size_t index, uint32_t* data)
            {
                bucket_t* b = getBucket(index);
                _encoder.decode(data, b->data, b->type);
#ifdef DEBUG
                assert(memcmp(data, b->org, sizeof(uint32_t)*_places) == 0);
#endif
                
            }
            
            uint32_t getFreeBucket()
            {
                if(_buckets.size()*STATESET_BUCKETS <= _freebucket)
                {
                    _buckets.push_back(new bucket_t[STATESET_BUCKETS]);                
                }
                
                return _freebucket + 1;
            }
            
            bucket_t* getBucket(uint32_t index)
            {
                if(index == 0) return NULL;
                index -= 1;
                return &_buckets[index/STATESET_BUCKETS][index%STATESET_BUCKETS];
            }

            bool nextWaiting(State* state)
            {
                if(_waiting < _freebucket)
                {
                    if(_encoding)
                    {
                        decompress(_waiting+1, state->marking());
                    }
                    else
                    {
                        memcpy( state->marking(), 
                                getBucket(_waiting+1)->data, 
                                _places*sizeof(uint32_t));
                    }
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

                nfree = getFreeBucket();
                auto result = _map.insert(mapnode_t(hash, nfree));
                bucket_t* bucket = getBucket(nfree);
                bucket->data = NULL;
                bucket->next = 0;                
                unsigned char type = bucket->type = _encoder.getType(sum, active, allsame, val);
                bucket->type += 32* last;
                assert(type == (bucket->type & 31));
                
                
                size_t size;
                const unsigned char* tomatch;
                if(_encoding)
                {
                    size = _encoder.encode(state->marking(), bucket->type);
                    tomatch = _encoder.scratchpad();
#ifdef DEBUG
                    uint32_t* tmp = new uint32_t[_places];
                    _encoder.decode(tmp, _encoder.scratchpad(), bucket->type);
                    for(size_t i = _places; i < 0; ++i) assert(tmp[i] == bucket->data[i]);
                    delete[] tmp;
#endif
                }
                else
                {
                    size = _places*sizeof(uint32_t);
                    tomatch = (unsigned char*) state->marking();
                }
                
                if(!result.second)
                {
                    bucket_t* old = getBucket(result.first->second);
                    while(old != NULL)
                    {
                        if( old->type == bucket->type &&
                            memcmp(old->data, tomatch, size) == 0)
                        {
#ifdef DEBUG
                            assert(memcmp(state->marking(), old->org, sizeof(uint32_t)*_places) == 0);
#endif
                            return false;
                        }
#ifdef DEBUG
                        assert(memcmp(state->marking(), old->org, sizeof(uint32_t)*_places) != 0);
#endif
                        old = getBucket(old->next);
                    }
                    bucket->next = result.first->second;
                    ++collisions;
                }

#ifdef DEBUG
                bucket->org = new uint32_t[_places];
                memcpy(bucket->org, state->marking(), sizeof(uint32_t)*_places);
#endif
                
                bucket->data = _allocator.allocate(size);
                                
                memcpy(bucket->data, tomatch, size);    
                
                assert(type < 32);
                if(_encoding) _encoderstats[type] += 1;
                                                
                result.first->second = nfree;  
                                
                ++_freebucket;                
                // update the max token bound for each place in the net (only for newly discovered markings)
                for (uint32_t i = 0; i < _places; i++) 
                {
                    _maxPlaceBound[i] = std::max<MarkVal>( state->marking()[i],
                                                            _maxPlaceBound[i]);
                }

#ifdef DEBUG
                if(_freebucket % 10000 == 1) std::cout << "Inserted number " << (_freebucket-1) << std::endl;
#endif
                
                if(!_encoding && _freebucket % 1000 == 0)
                {
                    size_t mem =    (STATESET_BUCKETS*sizeof(bucket_t))*_buckets.size() + 
                                    _allocator.memory()+
                                    _map.size()*sizeof(mapnode_t);
                    if(mem > _memorylimit)
                    {
                        switchToEncoding();
                    }
                }
                
                return true;
            }

            
            void switchToEncoding()
            {
                dataallocator_t nalloc;
                _encoding = true;
                for(size_t i = 1; i <= _freebucket; ++i)
                {
                    bucket_t* b = getBucket(i);
                    size_t size = _encoder.encode((uint32_t*)b->data, b->type);  
#ifdef DEBUG
                    uint32_t* tmp = new uint32_t[_places];
                    _encoder.decode(tmp, _encoder.scratchpad(), b->type);
                    for(size_t i = _places; i < 0; ++i) assert(tmp[i] == b->data[i]);
                    delete[] tmp;
#endif
                    _allocator.deallocate(b->data, _places*sizeof(uint32_t));
                    b->data = nalloc.allocate(size);
                    memcpy(b->data, _encoder.scratchpad(), size);
                    _encoderstats[b->type & 31] += 1;
                }
                _allocator = nalloc;
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
            uint32_t _places ;        
            std::unordered_map<uint32_t, uint32_t, std::hash<uint32_t>,
                std::equal_to<uint32_t>, mapallocator_t> _map;
            size_t _freebucket = 0;
            std::vector<bucket_t* > _buckets;
            size_t _waiting = 0;
            dataallocator_t _allocator;
            AlignedEncoder _encoder;
                    
            size_t _encoderstats[32];
            size_t _memorylimit;
            bool _encoding;
            size_t collisions = 0;
        };

    }
}


#endif // STATESET_H
