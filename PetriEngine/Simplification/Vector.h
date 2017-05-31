/* 
 * File:   LPFactory.h
 * Author: Peter G. Jensen
 *
 * Created on 31 May 2017, 09:26
 */


#include <vector>
#include <cstdlib>
#include "MurmurHash2.h"

#ifndef VECTOR_H
#define VECTOR_H

namespace PetriEngine {
    namespace Simplification {
        enum op_t 
        {
            OP_EQ,
            OP_LE,
            OP_GE,
            OP_LT,
            OP_GT,
            OP_NE
        };
        class LPCache;
        class Vector {
        public:
            friend LPCache;
            
            const int* data() const;
            
            void free();
            
            void inc();
            
            size_t size() const;

            bool operator ==(const Vector& other) const
            {
                return  _data == other._data;
            }
            
            size_t refs() const { return ref; }
            
        private:
            Vector(const std::vector<int>& data) : _data(data)
            {
                
            }

            std::vector<int> _data;
            LPCache* factory = NULL;
            size_t ref = 0;
        };
    }
}

namespace std
{
    using namespace PetriEngine::Simplification;
    
    template <>
    struct hash<Vector>
    {
        size_t operator()(const Vector& k) const
        {
            return MurmurHash64A(k.data(), 
                    k.size() * sizeof(int), 
                    1337);
        }
    };
}

#endif /* VECTOR_H */

