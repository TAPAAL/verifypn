/* 
 * File:   Encoder.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 11 March 2016, 14:15
 */

#include <limits>

#include "PetriEngine/Structures/AlignedEncoder.h"

#define SAMEBOUND 120
#define DBOUND (SAMEBOUND*2)

AlignedEncoder::AlignedEncoder(uint32_t places, uint32_t k)
: _places(places)
{

    size_t bytes = 2*sizeof(uint32_t) + (places*sizeof(uint32_t));
    _scratchpad = scratchpad_t(bytes*8);
    assert(_scratchpad.size() == (2*sizeof(uint32_t) + (_places*sizeof(uint32_t))));
    if(_places < 256) _psize = 1;
    else if(_places < 65536) _psize = 2;
    else _psize = 4;

    assert(_psize != 0);
}

AlignedEncoder::~AlignedEncoder()
{
    _scratchpad.release();
}

uint32_t AlignedEncoder::tokenBytes(uint32_t ntokens) const
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
    if(sizeof(T) == sizeof(uint32_t))
    {
        memcpy(&(_scratchpad.raw()[offset]), data, _places*sizeof(T));        
    } 
    else
    {
        for(size_t i = 0; i < _places; ++i)
        {
            T* dest = (T*)(&_scratchpad.raw()[offset + (i*sizeof(T))]);
            *dest = data[i];
        }
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
size_t AlignedEncoder::bitTokenCountsSize(const unsigned char* source, uint32_t offset) const
{
    scratchpad_t b = scratchpad_t((unsigned char*)&source[offset], _places);

    size_t cnt = 0;
    for(uint32_t i = 0; i < _places; ++i)
    {
        if(b.at(i))
        {
            cnt += sizeof(T);
        }
    }
    return offset + b.size() + cnt;
}

template<typename T>
uint32_t AlignedEncoder::readBitTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset) const
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
size_t AlignedEncoder::placeTokenCountsSize(const unsigned char* source, uint32_t offset) const
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
            size = std::numeric_limits<size_t>::max(); // should provoke an error
            assert(false);
    }
    offset += _psize;
    return offset + (_psize * size) + (size*sizeof(T));
}

template<typename T>
uint32_t AlignedEncoder::readPlaceTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset) const
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
            size = std::numeric_limits<size_t>::max(); // should provoke an error
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
            size = std::numeric_limits<size_t>::max(); // should provoke an error
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

unsigned char AlignedEncoder::getType(uint32_t sum, uint32_t pwt, bool same, uint32_t val) const
{
    if(pwt == 0) return 0;
    if(same && val <= SAMEBOUND)
    {
        size_t bvsize = scratchpad_t::bytes(_places);
        size_t indirect = _psize+pwt*_psize;
        
        if(bvsize <= indirect)
        {
            return val;
        }
        else
        {
            return SAMEBOUND+val;            
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
            return DBOUND+1;
        }
        else if(direct <= indirect && direct <= bvindirect)
        {
            switch(tsize)
            {
                case 1:
                    return DBOUND+2;       
                case 2:
                    return DBOUND+3;                 
                case 4:
                    return DBOUND+4;                
                default:
                    assert(false);
            }
        }
        else if(indirect <= bvindirect)
        {
            switch(tsize)
            {
                case 1:
                    return DBOUND+5;     
                case 2:
                    return DBOUND+6;                
                case 4:
                    return DBOUND+7;
                default:
                    assert(false);
            }
        } 
        else
        {
            switch(tsize)
            {
                case 1:
                    return DBOUND+8;                
                case 2:
                    return DBOUND+9;
                case 4:
                    return DBOUND+10;
                default:
                    assert(false);
            }
        }
    }
    assert(false);
    return 0;
}

size_t AlignedEncoder::size(const uchar* s) const
{
    unsigned char type = s[0];
    if(type <= SAMEBOUND)
    {
        if((_places % 8) == 0) return 1 + (_places / 8);
        else return 2 + (_places / 8);
    }
    
    if(type <= DBOUND)
    {
        size_t size;
        switch(_psize)
        {
            case 1:
                size = 1 + s[1];
                break;
            case 2:
                size = 1 + *(uint16_t*)(&s[1]);
                size *= sizeof(uint16_t);
                break;                           
            case 4:
                size = 1 + *(uint32_t*)(&s[1]);
                size *= sizeof(uint32_t);
                break;   
            default:
                size = std::numeric_limits<size_t>::max(); // should provoke an error
                assert(false);
        }
        return size + 1;
    }
    
    switch(type)
    {
        case DBOUND+1:
            if((_places % 4) == 0) return 1 + (_places / 4);
            else return 2 + (_places / 4);
        case DBOUND+2:
            return 1 + (sizeof(unsigned char)*_places);
        case DBOUND+3:
            return 1 + (sizeof(uint16_t)*_places);
        case DBOUND+4:
            return 1 + (sizeof(uint32_t)*_places);
        case DBOUND+5:
            return placeTokenCountsSize<unsigned char>((unsigned char*)s, 1); 
        case DBOUND+6:
            return placeTokenCountsSize<uint16_t>((unsigned char*)s, 1); 
        case DBOUND+7:
            return placeTokenCountsSize<uint32_t>((unsigned char*)s, 1); 
        case DBOUND+8:
            return bitTokenCountsSize<unsigned char>((unsigned char*)s, 1);
        case DBOUND+9:
            return bitTokenCountsSize<uint16_t>((unsigned char*)s, 1);
        case DBOUND+10:
            return bitTokenCountsSize<uint32_t>((unsigned char*)s, 1);
        default:
            assert(false);
            return std::numeric_limits<size_t>::infinity();
    }
}

size_t AlignedEncoder::encode(const uint32_t* d, unsigned char type)
{
    _scratchpad.zero();
    _scratchpad.raw()[0] = type;
    if(type <= SAMEBOUND)
    {
        return writeBitVector(1, d);
    }
    if(type <= DBOUND)
    {
        return writePlaces(1, d);
    }
    
    switch(type)
    {
        case DBOUND+1:
            return writeTwoBitVector(1,d);
        case DBOUND+2:
            return writeTokens<unsigned char>(1, d);           
        case DBOUND+3:
            return writeTokens<uint16_t>(1, d);
        case DBOUND+4:
            return writeTokens<uint32_t>(1, d); 
        case DBOUND+5:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<unsigned char>(size, d);
            }
        case DBOUND+6:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<uint16_t>(size, d);
            }
        case DBOUND+7:
            {
                size_t size = writePlaces(1, d);
                return writeTokenCounts<uint32_t>(size, d);
            }           
        case DBOUND+8:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<unsigned char>(size, d);
            }
        case DBOUND+9:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<uint16_t>(size, d);
            }
        case DBOUND+10:
            {
                size_t size = writeBitVector(1, d);
                return writeTokenCounts<uint32_t>(size, d);
            }
        default:
            assert(false);
    }
    assert(false);
    return 0;
}

void AlignedEncoder::decode(uint32_t* d, const unsigned char* s)
{
    memset(d, 0, sizeof(uint32_t)*_places);
    unsigned char type = s[0];
    if(type <= SAMEBOUND)
    {
        readBitVector(d, s, 1, type);
        return;
    }
    if(type <= DBOUND)
    {
        readPlaces(d, s, 1, type - SAMEBOUND);
        return;
    }
    
    switch(type)
    {
        case DBOUND+1:
            readTwoBitVector(d,s,1);
            return;
        case DBOUND+2:
            readTokens<unsigned char>(d,s,1);
            return;
        case DBOUND+3:
            readTokens<uint16_t>(d,s,1);
            return;
        case DBOUND+4:
            readTokens<uint32_t>(d,s,1);
            return;
        case DBOUND+5:
            readPlaceTokenCounts<unsigned char>(d, s, 1); 
            return;
        case DBOUND+6:
            readPlaceTokenCounts<uint16_t>(d, s, 1); 
            return;
        case DBOUND+7:
            readPlaceTokenCounts<uint32_t>(d, s, 1); 
            return;
        case DBOUND+8:
            readBitTokenCounts<unsigned char>(d, s, 1);
            return;
        case DBOUND+9:
            readBitTokenCounts<uint16_t>(d, s, 1);
            return;
        case DBOUND+10:
            readBitTokenCounts<uint32_t>(d, s, 1);
            return;
        default:
            assert(false);
    }
}
