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

/* 
 * File:   binarywrapper.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include <iostream>
#include <string.h>
#include <assert.h>
#include <limits>
#include <stdlib.h>
#include <stdint.h>

#ifndef BINARYWRAPPER_H
#define	BINARYWRAPPER_H

namespace ptrie
{
    typedef unsigned int uint;
    typedef unsigned char uchar;
    constexpr auto __BW_BSIZE__ = sizeof(size_t); // SIZE OF POINTER!
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
        
        binarywrapper_t() : _blob(nullptr), _nbytes(0)
        {            
        }
        
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
        
        inline void init(const binarywrapper_t& other, uint size, uint offset, 
                                                            uint encodingsize)
        {
            uint so = size + offset;
            offset = ((so - 1) / 8) - ((size - 1) / 8);

            _nbytes = ((encodingsize + this->overhead(encodingsize)) / 8);
            if (_nbytes > offset)
                _nbytes -= offset;
            else {
                _nbytes = 0;
            }

            _blob = allocate(_nbytes);

            memcpy(raw(), &(other.const_raw()[offset]), _nbytes);
        }
        
        
        binarywrapper_t(uchar* raw, uint size, uint offset, uint encsize);
	
	/**
         * Assign (not copy) raw data to pointer. Set number of bytes to size
         * @param org: some memory to point to
         * @param size: number of bytes.
         */
        
        binarywrapper_t(uchar* org, uint size);
        
        /**
         * Empty destructor. Does NOT deallocate data - do this with explicit
         * call to release().
         */
        
        ~binarywrapper_t()
        {        
        }
        
        /**
         * Makes a complete copy, including new heap-allocation
         * @return an exact copy, but in a different area of the heap.
         */
        
        //binarywrapper_t clone() const;

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
         * @param org: source data
         * @param size: number of bytes to copy
         */

        void copy(const uchar* org, uint size);

        // accessors
        /**
         * Get value of the place'th bit
         * @param place: bit index
         * @return 
         */
        inline bool at(const uint place) const
        {
            uint offset = place % 8;
            bool res2;
            if (place / 8 < _nbytes)
                res2 = (const_raw()[place / 8] & _masks[offset]) != 0;
            else
                res2 = false;

            return res2;  
        } 
        
        /**
         * number of bytes allocated in heap
         * @return 
         */
        
        inline uint size() const
        {
            return _nbytes;
        }
                
        /**
         * Raw access to data when in const setting
         * @return 
         */
        
        inline const uchar* const_raw() const
        {
            if(_nbytes <= __BW_BSIZE__) return offset((uchar*)&_blob, _nbytes);
            else 
                return offset(_blob, _nbytes);
        }
        
        /**
         * Raw access to data
         * @return 
         */
               
        inline uchar* raw()
        {
            return const_cast<uchar*>(const_raw());
        }

        /**
         * pretty print of content
         */
        
        void print(std::ostream& strean, size_t length = std::numeric_limits<size_t>::max()) const;
        
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
        
        inline void set(const uint place, const bool value)
        {
            assert(place < _nbytes*8);
            uint offset = place % 8;
            uint theplace = place / 8;
            if (value) {
                raw()[theplace] |= _masks[offset];
            } else {
                raw()[theplace] &= ~_masks[offset];
            }    
        }   
        
        /**
         * Sets all memory on heap to 0 
         */
        
        inline void zero()
        {
            if(_nbytes > 0 && _blob != nullptr)
            {
                memset(raw(), 0x0, _nbytes); 
            }
        }
        
        /**
         * Deallocates memory stored on heap
         */
        
        inline void release()
        {
            if(_nbytes > __BW_BSIZE__)
                dealloc(_blob);
            _blob = nullptr;
            _nbytes = 0;
        }
                
        /**
         * Nice access to single bits
         * @param i: index to access
         * @return 
         */

        inline uchar operator[](int i) const
        {
            if (i >= _nbytes) {
                 return 0x0;
            }
            return const_raw()[i]; 
        }
               
        
        /**
         * Compares two wrappers. Assumes that smaller number of bytes also means
         * a smaller wrapper. Otherwise compares byte by byte.
         * @param other: wrapper to compare to
         * @return -1 if other is smaller, 0 if same, 1 if other is larger
         */
        inline int cmp(const binarywrapper_t &other) const
        {
            if(_nbytes < other._nbytes) return -1;
            else if(_nbytes > other._nbytes) return 1;
            
            size_t bcmp = std::min(_nbytes, other._nbytes);
            return memcmp(const_raw(), other.const_raw(), bcmp);
        }
            
        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator==( const binarywrapper_t &enc1, 
                                const binarywrapper_t &enc2) {
            return enc1.cmp(enc2) == 0;
        }
        
        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator<(const binarywrapper_t &enc1, 
                               const binarywrapper_t &enc2) {
            return enc1.cmp(enc2) < 0;
        }

        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator!=(const binarywrapper_t &enc1, 
                               const binarywrapper_t &enc2) {
            return !(enc1 == enc2);
        }

        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator>=(const binarywrapper_t &enc1, 
                               const binarywrapper_t &enc2) {
            return !(enc1 < enc2);
        }
        
        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator>(const binarywrapper_t &enc1, 
                               const binarywrapper_t &enc2) {
            return enc2 < enc1;
        }

        /**
         * If sizes differs, the comparison is done here.
         * If sizes match, compares byte by byte.
         * @param enc1 
         * @param enc2
         * @return true if a match, false otherwise
         */        
        friend bool operator<=(const binarywrapper_t &enc1, 
                               const binarywrapper_t &enc2) {
            return enc2 < enc1;
        }
        
	const static uchar _masks[8];
    private:
         
        static inline uchar* allocate(size_t n)
        {
            if(n <= __BW_BSIZE__) return 0;
#ifndef NDEBUG
            size_t on = n;
#endif            
            if(n % __BW_BSIZE__ != 0) n = (1+(n/__BW_BSIZE__))*(__BW_BSIZE__);
            assert(n % __BW_BSIZE__ == 0);
            assert(on <= n);
            return (uchar*)malloc(n);
        }
        
        static inline uchar* zallocate(size_t n)
        {
            if(n <= __BW_BSIZE__) return 0;
#ifndef NDEBUG
            size_t on = n;
#endif
            if(n % __BW_BSIZE__ != 0)
            {
                n = (1+(n/__BW_BSIZE__))*(__BW_BSIZE__);
                assert(n == on + (__BW_BSIZE__ - (on % __BW_BSIZE__)));
            }
            assert(n % __BW_BSIZE__ == 0);
            assert(on <= n);
            return (uchar*)calloc(n, 1);            
        }
        
        static inline void dealloc(uchar* data)
        {
            free(data);
        }
        
        static inline uchar* offset(uchar* data, uint16_t size)
        {
//            if((size % __BW_BSIZE__) == 0) return data;
//            else return &data[(__BW_BSIZE__ - (size % __BW_BSIZE__))];
            return data;
        }
        
        // blob of heap-allocated data
        uchar* _blob = nullptr;
            
        // number of bytes allocated on heap
        uint16_t _nbytes = 0;
               
        // masks for single-bit access
     } __attribute__((packed));
}
namespace std {
    std::ostream &operator<<(std::ostream &os, const ptrie::binarywrapper_t &b);
}
#endif	/* BINARYWRAPPER_H */

