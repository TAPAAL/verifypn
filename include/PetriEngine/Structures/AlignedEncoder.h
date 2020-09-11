/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on 11 March 2016, 14:15
 */


#ifndef ALIGNEDENCODER_H
#define	ALIGNEDENCODER_H

#include <cmath>
#include "binarywrapper.h"

using namespace ptrie;


class AlignedEncoder
{
    typedef binarywrapper_t scratchpad_t;
    public:
        AlignedEncoder(uint32_t places, uint32_t k);

        ~AlignedEncoder();

        size_t encode(const uint32_t* data, unsigned char type);

        void decode(uint32_t* destination, const unsigned char* source);
     
        binarywrapper_t& scratchpad()
        {
            return _scratchpad;
        }
        
        unsigned char getType(uint32_t sum, uint32_t pwt, bool same, uint32_t val) const;
        
        size_t size(const uchar* data) const;
private:
        uint32_t tokenBytes(uint32_t ntokens) const;
    
        uint32_t writeBitVector(size_t offset, const uint32_t* data);
        
        uint32_t writeTwoBitVector(size_t offset, const uint32_t* data);
        
        template<typename T>
        uint32_t writeTokens(size_t offset, const uint32_t* data);
        
        template<typename T>
        uint32_t writeTokenCounts(size_t offset, const uint32_t* data);
        
        uint32_t writePlaces(size_t offset, const uint32_t* data);
                
        uint32_t readBitVector(uint32_t* destination, const unsigned char* source, uint32_t offset, uint32_t value);

        uint32_t readTwoBitVector(uint32_t* destination, const unsigned char* source, uint32_t offset);
        
        uint32_t readPlaces(uint32_t* destination, const unsigned char* source, uint32_t offset, uint32_t value);
                
        template<typename T>
        uint32_t readTokens(uint32_t* destination, const unsigned char* source, uint32_t offset);
        
        template<typename T>
        uint32_t readPlaceTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset) const;

        template<typename T>        
        size_t placeTokenCountsSize(const unsigned char* source, uint32_t offset) const;
        
        template<typename T>
        uint32_t readBitTokenCounts(uint32_t* destination, const unsigned char* source, uint32_t offset) const;

        template<typename T>        
        size_t bitTokenCountsSize(const unsigned char* source, uint32_t offset) const;
        
        uint32_t _places;
        
        uint32_t _psize;
     
        // dummy value for template
        scratchpad_t _scratchpad;

};


#endif	/* ALIGNEDENCODER_H */
