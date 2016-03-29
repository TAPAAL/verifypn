/* 
 * File:   binarywrapper.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include <math.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "Memory/pool_dynamic_allocator.h"

#ifndef BINARYWRAPPER_H
#define	BINARYWRAPPER_H
namespace ptrie
{
    typedef unsigned int uint;
    typedef unsigned char uchar;
    
    /**
     * Wrapper for binary data. This provides easy access to individual bits, 
     * heap allocation and comparison. Notice that one has to make sure to 
     * explicitly call release() if one wishes to deallocate (possibly shared data).
     * 
     */
    template<class T>
    class binarywrapper_t
    {
    public:
        // Constructors
        /**
         * Empty constructor, no data is allocated
         */
        inline
        binarywrapper_t();        
        
        /**
         Allocates a room for at least size bits
         */
        inline
        binarywrapper_t(uint size);
        
        /**
         * Constructor for copying over data from latest the offset'th bit.
         * Detects overflows.
         * @param other: wrapper to copy from
         * @param offset: maximal number of bits to skip.
         */
        inline
        binarywrapper_t(const binarywrapper_t& other, uint offset);
        binarywrapper_t(const binarywrapper_t& other, uint size, uint offset, 
                                                            uint encodingsize,
                pdalloc::pool_dynamic_allocator<unsigned char>& alloc);
        binarywrapper_t(uchar* raw, uint size, uint offset, uint encsize);
	
	/**
         * Assign (not copy) raw data to pointer. Set number of bytes to size
         * @param raw: some memory to point to
         * @param size: number of bytes.
         */
        inline
        binarywrapper_t(uchar* raw, uint size);
        
        /**
         * Empty destructor. Does NOT deallocate data - do this with explicit
         * call to release().
         */
        inline
        ~binarywrapper_t();
        
        /**
         * Makes a complete copy, including new heap-allocation
         * @return an exact copy, but in a different area of the heap.
         */
        inline
        binarywrapper_t clone() const;
        
        /**
         * Copy over data and meta-data from other, but insert only into target
         * after offset bits.
         * Notice that this can cause memory-corruption if there is not enough
         * room in target, or to many bits are skipped.
         * @param other: wrapper to copy from
         * @param offset: bits to skip 
         */
        inline
        void copy(const binarywrapper_t& other, uint offset);
        
        /**
         * Copy over size bytes form raw data. Assumes that current wrapper has
         * enough room.
         * @param raw: source data
         * @param size: number of bytes to copy
         */
        inline
        void copy(const uchar* raw, uint size);
        
        // accessors
        /**
         * Get value of the place'th bit
         * @param place: bit index
         * @return 
         */
        inline
        bool at(const uint place) const;
        
        /**
         * number of bytes allocated in heap
         * @return 
         */
        inline
        uint size() const;
        
        /**
         * Raw access to data
         * @return 
         */
        inline
        uchar*& raw();
        
        /**
         * Raw access to data when in const setting
         * @return 
         */
        inline
        uchar* const_raw() const;
        
        /**
         * pretty print of content
         */
        inline
        void print() const;
        
        /**
         * finds the overhead (unused number of bits) when allocating for size
         * bits.
         * @param size: number of bits
         * @return 
         */
        inline
        static size_t overhead(uint size);
        
        inline
        static size_t bytes(uint size);
        // modifiers
        /**
         * Change value of place'th bit 
         * @param place: index of bit to change
         * @param value: desired value
         */
        inline
        void set(const uint place, const bool value) const;
        
        /**
         * Sets all memory on heap to 0 
         */
        inline
        void zero() const;
        
        /**
         * Deallocates memory stored on heap
         */
        inline
        void release();

        void set_meta(T data);
        T const_get_meta() const;
        T &get_meta();
                
        /**
         * Nice access to single bits
         * @param i: index to access
         * @return 
         */
        inline
        uchar operator[](int i);
        
        /**
         * Removes a number of bytes from end of heap-allocated data if any is 
         * allocated nothing happens if not. Bound-checks.
         * @param number of bytes to remove.
         */
        inline
        void pop_front(unsigned short, pdalloc::pool_dynamic_allocator<unsigned char>&);
        
        /**
         * Compares two wrappers. Assumes that smaller number of bytes also means
         * a smaller wrapper. Otherwise compares byte by byte.
         * @param other: wrapper to compare to
         * @return -1 if other is smaller, 0 if same, 1 if other is larger
         */
        inline int cmp(const binarywrapper_t &other) const
        {
            if(_nbytes != other._nbytes)
            {
                if(_nbytes < other._nbytes) return -1;
                else return 1;
            }
                
            return memcmp(_blob, other.const_raw(), other._nbytes );
        }
            
        /**
         * Compares wrappers bytes by bytes. If sizes do not match, they are not
         * equal. If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */
        inline friend bool operator==(  const binarywrapper_t &enc1, 
                                        const binarywrapper_t &enc2) {
            return enc1.cmp(enc2) == 0;
        }
        
    private:
            
        // blob of heap-allocated data
        uchar* _blob;
            
        // number of bytes allocated on heap
         unsigned short _nbytes;
        
        // meta data to carry
        T _meta;
            
        // masks for single-bit access
        const static uchar _masks[8];
    };
    
    template<class T>
    const uchar binarywrapper_t<T>::_masks[8] = {
        static_cast <uchar>(0x01),
        static_cast <uchar>(0x02),
        static_cast <uchar>(0x04),
        static_cast <uchar>(0x08),
        static_cast <uchar>(0x10),
        static_cast <uchar>(0x20),
        static_cast <uchar>(0x40),
        static_cast <uchar>(0x80)
    };
            
    template<class T>
    size_t binarywrapper_t<T>::overhead(uint size)
    {
        size = size % 8;
        if (size == 0)
            return 0;
        else
            return 8 - size; 
    }
    
    template<class T>
    size_t binarywrapper_t<T>::bytes(uint size)
    {
        return (size + overhead(size))/8;
    }
    
    template<class T>
    binarywrapper_t<T>::~binarywrapper_t()
    {
        
    }
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t()
    {}
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t(uint size)
    {
        _nbytes = (size + overhead(size)) / 8;
        _blob = new uchar[_nbytes];
        memset(_blob, 0x0, _nbytes);
    }
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t(const binarywrapper_t& other, uint offset)
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
        set_meta(other.const_get_meta());
    }
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t(
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
        set_meta(other.const_get_meta());
    }
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t
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
    
    template<class T>
    binarywrapper_t<T>::binarywrapper_t(uchar* raw, uint size)
    {
        _blob = raw;
        _nbytes = size / 8 + (size % 8 ? 1 : 0);     
    }
    
    // Copy and clones
    template<class T>
    binarywrapper_t<T> binarywrapper_t<T>::clone() const
    {
        binarywrapper_t<T> s;
        s._nbytes = _nbytes;
        s._blob = new uchar[_nbytes];
        memcpy(s._blob, _blob, _nbytes);
        s._meta = _meta;
        return s; 
    }
    
    template<class T>
    void binarywrapper_t<T>::copy(const binarywrapper_t& other, uint offset)
    {
        memcpy(&(_blob[offset / 8]), other._blob, other._nbytes);
        _meta = other._meta;
    }
    
    template<class T>
    void binarywrapper_t<T>::copy(const uchar* raw, uint size)
    {
        if(size > 0)
        {
            _blob = new char[size];
            memcpy(_blob, raw, size);
        }
    }
        
    // accessors
    template<class T>
    bool binarywrapper_t<T>::at(const uint place) const
    {
        uint offset = place % 8;
        bool res2;
        if (place / 8 < _nbytes)
            res2 = (_blob[place / 8] & _masks[offset]) != 0;
        else
            res2 = false;

        return res2;  
    }
    
    template<class T>
    uint binarywrapper_t<T>::size() const
    {
        return _nbytes;
    }
    
    template<class T>
    uchar*& binarywrapper_t<T>::raw()
    {
        return _blob; 
    }
    
    template<class T>
    uchar* binarywrapper_t<T>::const_raw() const
    {
        return _blob; 
    }
    
    template<class T>
    void binarywrapper_t<T>::print() const
    {
        for (size_t i = 0; i < _nbytes * 8; i++)
                std::cout << this->at(i);
            std::cout << std::endl;
    }
    
    
    template<class T>
    void binarywrapper_t<T>::pop_front(unsigned short int topop, pdalloc::pool_dynamic_allocator<unsigned char>& alloc)
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
    
    template<class T>
    void binarywrapper_t<T>::set(const uint place, const bool value) const
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
    
    template<class T>
    void binarywrapper_t<T>::zero() const
    {
        if(_nbytes > 0 && _blob != NULL)
        {
            memset(_blob, 0x0, _nbytes); 
        }
    }
    
    template<class T>
    void binarywrapper_t<T>::release()
    {
        delete[] _blob;
        _blob = NULL;
    }
    
    template<class T>
    void binarywrapper_t<T>::set_meta(T data)
    {
        _meta = data;
    }
    
    template<class T>
    T binarywrapper_t<T>::const_get_meta() const
    {
        return _meta;
    }
    
    template<class T>
    T& binarywrapper_t<T>::get_meta()
    {
        return _meta;
    }
    
    template<class T>
    uchar binarywrapper_t<T>::operator[](int i)
    {
       if (i >= _nbytes) {
            return 0x0;
        }
        return _blob[i]; 
    }
}


#endif	/* BINARYWRAPPER_H */

