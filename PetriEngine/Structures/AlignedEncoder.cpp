/* 
 * File:   Encoder.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 11 March 2016, 14:15
 */

#include <limits>

#include "AlignedEncoder.h"


AlignedEncoder::AlignedEncoder(uint32_t places, uint32_t k)
: _places(places)
{

    size_t bytes = 2*sizeof(uint32_t) + (places*sizeof(uint32_t));
    _scratchpad = scratchpad_t(bytes*8);

    if(_places < 256) _psize = 1;
    else if(_places < 65536) _psize = 2;
    else _psize = 4;

    assert(_psize != 0);
}

AlignedEncoder::~AlignedEncoder()
{
    _scratchpad.release();
}

uint32_t AlignedEncoder::tokenBytes(uint32_t ntokens)
{
    uint32_t size = 0;
    if(ntokens < 256) size = 1;
    else if(ntokens < 65536) size = 2;
    else size = 4;
    return size;
}

uint32_t AlignedEncoder::writeBitVector(size_t offset, const uint32_t* data)
{
    for(size_t i = 0; i < _places; ++i)
    {
        _scratchpad.set(i+(offset*8), data[i] > 0);
    }
    return offset + scratchpad_t::bytes(_places);
}

uint32_t AlignedEncoder::writeTwoBitVector(size_t offset, const uint32_t* data)
{
    for(size_t i = 0; i < _places; ++i)
    {
        switch(data[i])
        {
            case 1:
                _scratchpad.set((i*2)+(offset*8), true);
                break;
            case 3:
                _scratchpad.set((i*2)+(offset*8), true);                
            case 2:
                _scratchpad.set((i*2)+(offset*8)+1, true);                                
                break;
            default:                
                break;
            
        }
    }

    return offset + scratchpad_t::bytes(_places*2);
}

uint32_t AlignedEncoder::readTwoBitVector(uint32_t* destination, const unsigned char* source, uint32_t offset)
{
    scratchpad_t b = scratchpad_t((unsigned char*)&source[offset], _places*2);
    for(size_t i = 0; i < _places; ++i)
    {
        destination[i] = 0;
        if(b.at((i*2)))
        {
            destination[i] = 1;
        }
        
        if(b.at((i*2)+1))
        {
            destination[i] += 2;
        }
    }
    return offset + scratchpad_t::bytes(_places*2);
}

template<typename T>
uint32_t AlignedEncoder::writeTokens(size_t offset, const uint32_t* data)
{
    for(size_t i = 0; i < _places; ++i)
    {
        T* dest = (T*)(&_scratchpad.raw()[offset + (i*sizeof(T))]);
        *dest = data[i];
    }
    return offset + _places*sizeof(T);
}

template<typename T>
uint32_t AlignedEncoder::readTokens(uint32_t* destination, const unsigned char* source, uint32_t offset)
{
    
    for(size_t i = 0; i < _places; ++i)
    {
        T* src = (T*)(&source[offset + (i*sizeof(T))]);
        destination[i] = *src;
    }
    return offset + _places*sizeof(T);
}

template<typename T>
uint32_t AlignedEncoder::writeTokenCounts(size_t offset, const uint32_t* data)
{
    size_t cnt = 0;

    for(size_t i = 0; i < _places; ++i)
    {
        if(data[i] > 0)
        {
            T* dest = (T*)(&_scratchpad.raw()[offset + (cnt*sizeof(T))]);
            *dest = data[i];
            ++cnt;
        }
    }
    return offset + cnt*sizeof(T);
}

template<typename T>
uint32_t AlignedEncoder::readBitTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset)
{
    const unsigned char* ts = &source[offset + scratchpad_t::bytes(_places)];
    scratchpad_t b = scratchpad_t((unsigned char*)&source[offset], _places);

    size_t cnt = 0;
    for(uint32_t i = 0; i < _places; ++i)
    {
        if(b.at(i))
        {
            destination[i] = *((T*)&ts[cnt]);
            cnt += sizeof(T);
        }
    }
    return 0;
}

template<typename T>
uint32_t AlignedEncoder::readPlaceTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset)
{
    size_t size;
    switch(_psize)
    {
        case 1:
            size = source[offset];
            break;
        case 2:
            size = *(uint16_t*)(&source[offset]);
            break;                           
        case 4:
            size = *(uint32_t*)(&source[offset]);
            break;   
        default:
            assert(false);
    }
    
    offset += _psize;
    const unsigned char* ts = &source[offset + (_psize*size)];
    for(size_t i = 0; i < size; ++i)
    {
        size_t pos = 0;
        switch(_psize)
        {
            case 1:
                pos = source[offset + i];
                break;                
            case 2:
                pos = *((uint16_t*)&source[offset + i*2]);
                break;                
            case 4:
                pos = *((uint32_t*)&source[offset + i*4]);
                break;
            default:
                assert(false);
                break;
        }
        
        destination[pos] = *((T*)&ts[i*sizeof(T)]);     
    }
    return offset + size;
}

uint32_t AlignedEncoder::writePlaces(size_t offset, const uint32_t* data)
{
    size_t cnt = 0;
    uint16_t* dest16 = (uint16_t*)(&_scratchpad.raw()[offset]);
    uint32_t* dest32 = (uint32_t*)(&_scratchpad.raw()[offset]);
    for(size_t i = 0; i < _places; ++i)
    {
        if(data[i] > 0)
        {
            switch(_psize)
            {
                case 1:
                    _scratchpad.raw()[offset + cnt + 1] = (unsigned char)i;
                    break;
                case 2:
                    dest16[cnt+1] = (uint16_t)i;
                    break;  
                case 4:
                    dest32[cnt+1] = i;
                    break;                   
                default:
                    assert(false);
            }
            ++cnt;
        }
        
    }
    
    switch(_psize)
    {
        case 1:
            _scratchpad.raw()[offset] = (unsigned char)cnt;
            break;
        case 2:
            dest16[0] = cnt;
            break;            
        case 4:
            dest32[0] = cnt;
            break;
        default:
            assert(false);
    }
    return offset + _psize + cnt*_psize; 
}

uint32_t AlignedEncoder::readPlaces(uint32_t* destination, const unsigned char* source, uint32_t offset, uint32_t value)
{
    size_t size;
    switch(_psize)
    {
        case 1:
            size = source[offset];
            break;
        case 2:
            size = *(uint16_t*)(&source[offset]);
            break;                           
        case 4:
            size = *(uint32_t*)(&source[offset]);
            break;   
        default:
            assert(false);
    }
    
    offset += _psize;
    
    uint16_t* raw16 = (uint16_t*) &source[offset];
    uint32_t* raw32 = (uint32_t*) &source[offset];
    for(size_t i = 0; i < size; ++i)
    {
        switch(_psize)
        {
            case 1:
                destination[source[i+offset]] = value;
                break;
            case 2:
                 destination[raw16[i]] = value;
                break;
            case 4:
                destination[raw32[i]] = value;
                break;                
            default:
                assert(false);
        }
    }
    return offset + _psize*size;
}

uint32_t AlignedEncoder::readBitVector(uint32_t* destination, const unsigned char* source, uint32_t offset, uint32_t value)
{
    scratchpad_t b = scratchpad_t((unsigned char*)&source[offset], _places);
    for(uint32_t i = 0; i < _places; ++i)
    {
        if(b.at(i))
        {
            destination[i] = value;
        }
        else
        {
            destination[i] = 0;
        }
    }
    return offset + b.size();
}

unsigned char AlignedEncoder::getType(uint32_t sum, uint32_t pwt, bool same, uint32_t val)
{
    if(pwt == 0) return 0;
    if(same && val < 11)
    {
        size_t bvsize = scratchpad_t::bytes(_places);
        size_t indirect = _psize+pwt*_psize;
        
        if(bvsize <= indirect)
        {
            return val;
        }
        else
        {
            return 10+val;            
        }
    }
    else
    {
        size_t tsize = tokenBytes(val);
        size_t bvsize = scratchpad_t::bytes(_places*2);
        size_t indirect = _psize+pwt*(_psize+tsize);   
        size_t bvindirect = scratchpad_t::bytes(_places)+pwt*tsize;
        size_t direct = _places*tsize;
        
        if(val < 4 && bvsize <= indirect && bvsize <= bvindirect)
        {
            return 22;
        }
        else if(direct <= indirect && direct <= bvindirect)
        {
            switch(tsize)
            {
                case 1:
                    return 23;       
                case 2:
                    return 24;                 
                case 4:
                    return 25;                
                default:
                    assert(false);
            }
        }
        else if(indirect <= bvindirect)
        {
            switch(tsize)
            {
                case 1:
                    return 26;     
                case 2:
                    return 27;                
                case 4:
                    return 28;
                default:
                    assert(false);
            }
        } 
        else
        {
            switch(tsize)
            {
                case 1:
                    return 29;                
                case 2:
                    return 30;
                case 4:
                    return 31;
                default:
                    assert(false);
            }
        }
    }
    assert(false);
}

size_t AlignedEncoder::encode(const uint32_t* d, unsigned char type)
{
    _scratchpad.zero();
    _scratchpad.raw()[0] = type;
    type &= 31; // remove everything else than pure type
    if(type < 11)
    {
        return writeBitVector(1, d);
    }
    if(type < 22)
    {
        return writePlaces(1, d);
    }
    
    switch(type)
    {
        case 22:
            return writeTwoBitVector(1,d);
        case 23:
            return writeTokens<unsigned char>(1, d);           
        case 24:
            return writeTokens<uint16_t>(1, d);
        case 25:
            {
                uint32_t* raw = (uint32_t*)_scratchpad.raw();
                memcpy(raw, d, _places*sizeof(uint32_t));
            }
            return writeTokens<uint32_t>(1, d); 
        case 26:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<unsigned char>(size, d);
            }
        case 27:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<uint16_t>(size, d);
            }
        case 28:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<uint32_t>(size, d);
            }           
        case 29:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<unsigned char>(size, d);
            }
        case 30:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<uint16_t>(size, d);
            }
        case 31:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<uint32_t>(size, d);
            }
        default:
            assert(false);
    }
}

void AlignedEncoder::decode(uint32_t* d, const unsigned char* s)
{
    memset(d, 0, sizeof(uint32_t)*_places);
    unsigned char type = s[0];
    if(type < 11)
    {
        readBitVector(d, s, 1, type);
        return;
    }
    if(type < 22)
    {
        readPlaces(d, s, 1, type-10);
        return;
    }
    
    switch(type)
    {
        case 22:
            readTwoBitVector(d,s,1);
            return;
        case 23:
            readTokens<unsigned char>(d,s,1);
            return;
        case 24:
            readTokens<uint16_t>(d,s,1);
            return;
        case 25:
            memcpy(d, s, _places*sizeof(uint32_t));
            return;
        case 26:
            readPlaceTokenCounts<unsigned char>(d, s, 1); 
            return;
        case 27:
            readPlaceTokenCounts<uint16_t>(d, s, 1); 
            return;
        case 28:
            readPlaceTokenCounts<uint32_t>(d, s, 1); 
            return;
        case 29:
            readBitTokenCounts<unsigned char>(d, s, 1);
            return;
        case 30:
            readBitTokenCounts<uint16_t>(d, s, 1);
            return;
        case 31:
            readBitTokenCounts<uint32_t>(d, s, 1);
            return;
        default:
            assert(false);
    }
}
