/* 
 * File:   binarywrapper.cpp
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include "binarywrapper.h"
namespace ptrie
{
    const uchar binarywrapper_t::_masks[8] = {
        static_cast <uchar>(0x01),
        static_cast <uchar>(0x02),
        static_cast <uchar>(0x04),
        static_cast <uchar>(0x08),
        static_cast <uchar>(0x10),
        static_cast <uchar>(0x20),
        static_cast <uchar>(0x40),
        static_cast <uchar>(0x80)
    };
            
    size_t binarywrapper_t::overhead(uint size)
    {
        size = size % 8;
        if (size == 0)
            return 0;
        else
            return 8 - size; 
    }
    
    
    size_t binarywrapper_t::bytes(uint size)
    {
        return (size + overhead(size))/8;
    }
    
    
    binarywrapper_t::~binarywrapper_t()
    {
        
    }
    
    
    binarywrapper_t::binarywrapper_t()
    {}
    
    
    binarywrapper_t::binarywrapper_t(uint size)
    {
        _nbytes = (size + overhead(size)) / 8;
        _blob = new uchar[_nbytes];
        memset(_blob, 0x0, _nbytes);
    }
    
    
    binarywrapper_t::binarywrapper_t(const binarywrapper_t& other, uint offset)
    {
         offset = offset / 8;

        _nbytes = other._nbytes;
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
        }

        _blob = new uchar[_nbytes];
        memcpy(_blob, &(other._blob[offset]), _nbytes);
    }
    
    
    binarywrapper_t::binarywrapper_t(
        const binarywrapper_t& other, uint size, uint offset, uint encodingsize,
            pdalloc::pool_dynamic_allocator<unsigned char>& alloc)
    {
        uint so = size + offset;
        offset = ((so - 1) / 8) - ((size - 1) / 8);

        _nbytes = ((encodingsize + this->overhead(encodingsize)) / 8);
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
        }

        _blob = alloc.allocate(_nbytes);
        memcpy(_blob, &(other._blob[offset]), _nbytes);
    }
    
    
    binarywrapper_t::binarywrapper_t
        (uchar* raw, uint size, uint offset, uint encodingsize)
    {
        
        uint so = size + offset;
        offset = ((so - 1) / 8) - ((size - 1) / 8);

        _nbytes = ((encodingsize + this->overhead(encodingsize)) / 8);
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
        }

        _blob = &(raw[offset]);
    }
    
    
    binarywrapper_t::binarywrapper_t(uchar* raw, uint size)
    {
        _blob = raw;
        _nbytes = size / 8 + (size % 8 ? 1 : 0);     
    }
    
    // Copy and clones
    
    binarywrapper_t binarywrapper_t::clone() const
    {
        binarywrapper_t s;
        s._nbytes = _nbytes;
        s._blob = new uchar[_nbytes];
        memcpy(s._blob, _blob, _nbytes);
        return s; 
    }
    
    
    void binarywrapper_t::copy(const binarywrapper_t& other, uint offset)
    {
        memcpy(&(_blob[offset / 8]), other._blob, other._nbytes);
    }
    
    
    void binarywrapper_t::copy(const uchar* raw, uint size)
    {
        if(size > 0)
        {
            _blob = new uchar[size];
            memcpy(_blob, raw, size);
        }
    }
        
    // accessors
    
    bool binarywrapper_t::at(const uint place) const
    {
        uint offset = place % 8;
        bool res2;
        if (place / 8 < _nbytes)
            res2 = (_blob[place / 8] & _masks[offset]) != 0;
        else
            res2 = false;

        return res2;  
    }
    
    
    uint binarywrapper_t::size() const
    {
        return _nbytes;
    }
    
    
    uchar* binarywrapper_t::raw()
    {
        return _blob; 
    }
    
    
    uchar* binarywrapper_t::const_raw() const
    {
        return _blob; 
    }
    
    
    void binarywrapper_t::print() const
    {
        for (size_t i = 0; i < _nbytes * 8; i++)
                std::cout << this->at(i);
            std::cout << std::endl;
    }
        
    void binarywrapper_t::pop_front(unsigned short int topop, pdalloc::pool_dynamic_allocator<unsigned char>& alloc)
    {
        if(_nbytes == 0) return;  // Special case, nothing to do!
        unsigned short int nbytes;
        
        // make sure we do not remove to much, but as much as we can.
        if(topop >= _nbytes)
        {
            topop = _nbytes;
            nbytes = 0;
        }
        else
        {
            nbytes = _nbytes - topop;            
        }
        
        if(nbytes > 0)
        {
            uchar* tmpblob = alloc.allocate(nbytes);
            memcpy(tmpblob, &(_blob[topop]), (nbytes));
            alloc.deallocate(_blob, _nbytes);
            _blob = tmpblob;
        }
        else
        {
            alloc.deallocate(_blob, _nbytes);
            _blob = NULL;
        }
        _nbytes = nbytes;
    }
    
    
    void binarywrapper_t::set(const uint place, const bool value) const
    {
        assert(place < _nbytes*8);
        uint offset = place % 8;
        uint theplace = place / 8;
        if (value) {
            _blob[theplace] |= _masks[offset];
        } else {
            _blob[theplace] &= ~_masks[offset];
        }    
    }
    
    
    void binarywrapper_t::zero() const
    {
        if(_nbytes > 0 && _blob != NULL)
        {
            memset(_blob, 0x0, _nbytes); 
        }
    }
    
    
    void binarywrapper_t::release()
    {
        delete[] _blob;
        _blob = NULL;
    }
    
    uchar binarywrapper_t::operator[](int i)
    {
       if (i >= _nbytes) {
            return 0x0;
        }
        return _blob[i]; 
    }
}