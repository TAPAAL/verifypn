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
    class binarywrapper_t
    {
    public:
        // Constructors
        /**
         * Empty constructor, no data is allocated
         */
        
        binarywrapper_t();        
        
        /**
         Allocates a room for at least size bits
         */
        
        binarywrapper_t(uint size);
        
        /**
         * Constructor for copying over data from latest the offset'th bit.
         * Detects overflows.
         * @param other: wrapper to copy from
         * @param offset: maximal number of bits to skip.
         */
        
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
        
        binarywrapper_t(uchar* raw, uint size);
        
        /**
         * Empty destructor. Does NOT deallocate data - do this with explicit
         * call to release().
         */
        
        ~binarywrapper_t();
        
        /**
         * Makes a complete copy, including new heap-allocation
         * @return an exact copy, but in a different area of the heap.
         */
        
        binarywrapper_t clone() const;
        
        /**
         * Copy over data and meta-data from other, but insert only into target
         * after offset bits.
         * Notice that this can cause memory-corruption if there is not enough
         * room in target, or to many bits are skipped.
         * @param other: wrapper to copy from
         * @param offset: bits to skip 
         */
        
        void copy(const binarywrapper_t& other, uint offset);
        
        /**
         * Copy over size bytes form raw data. Assumes that current wrapper has
         * enough room.
         * @param raw: source data
         * @param size: number of bytes to copy
         */
        
        void copy(const uchar* raw, uint size);
        
        // accessors
        /**
         * Get value of the place'th bit
         * @param place: bit index
         * @return 
         */
        bool at(const uint place) const;
        
        /**
         * number of bytes allocated in heap
         * @return 
         */
        
        uint size() const;
        
        /**
         * Raw access to data
         * @return 
         */
        
        uchar* raw();
        
        /**
         * Raw access to data when in const setting
         * @return 
         */
        
        uchar* const_raw() const;
        
        /**
         * pretty print of content
         */
        
        void print() const;
        
        /**
         * finds the overhead (unused number of bits) when allocating for size
         * bits.
         * @param size: number of bits
         * @return 
         */
        
        static size_t overhead(uint size);
        
        
        static size_t bytes(uint size);
        // modifiers
        /**
         * Change value of place'th bit 
         * @param place: index of bit to change
         * @param value: desired value
         */
        
        void set(const uint place, const bool value) const;
        
        /**
         * Sets all memory on heap to 0 
         */
        
        void zero() const;
        
        /**
         * Deallocates memory stored on heap
         */
        
        void release();
                
        /**
         * Nice access to single bits
         * @param i: index to access
         * @return 
         */
        
        uchar operator[](int i);
        
        /**
         * Removes a number of bytes from end of heap-allocated data if any is 
         * allocated nothing happens if not. Bound-checks.
         * @param number of bytes to remove.
         */
        
        void pop_front(unsigned short, pdalloc::pool_dynamic_allocator<unsigned char>&);
        
        /**
         * Compares two wrappers. Assumes that smaller number of bytes also means
         * a smaller wrapper. Otherwise compares byte by byte.
         * @param other: wrapper to compare to
         * @return -1 if other is smaller, 0 if same, 1 if other is larger
         */
         int cmp(const binarywrapper_t &other) const
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
         friend bool operator==(  const binarywrapper_t &enc1, 
                                        const binarywrapper_t &enc2) {
            return enc1.cmp(enc2) == 0;
        }
        
    private:
            
        // blob of heap-allocated data
        uchar* _blob;
            
        // number of bytes allocated on heap
         unsigned short _nbytes;
        
        // masks for single-bit access
        const static uchar _masks[8];
    } __attribute__((packed));
}


#endif	/* BINARYWRAPPER_H */

