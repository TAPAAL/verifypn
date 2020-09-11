/* 
 * File:   LPFactory.h
 * Author: Peter G. Jensen
 *
 * Created on 31 May 2017, 09:26
 */


#include <vector>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
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
                        
            void free();
            
            void inc();
            
            size_t data_size() const;

            bool operator ==(const Vector& other) const
            {
                return  _data == other._data;
            }
            
            
            
            const void* raw() const
            {
                return _data.data();
            }
            
            size_t refs() const { return ref; }
            
            std::ostream& print(std::ostream& ss) const
            {
                int index = 0;
                for(const std::pair<int,int>& el : _data)
                {
                    while(index < el.first) { ss << "0 "; ++index; }
                    ss << el.second << " ";
                    ++index;
                }
                return ss;
            }
            
            void write(std::vector<double>& dest) const
            {
                memset(dest.data(), 0, sizeof (double) * dest.size());
                
                for(const std::pair<int,int>& el : _data)
                {
                    dest[el.first + 1] = el.second;
                }
            }

            size_t write_indir(std::vector<double>& dest, std::vector<int32_t>& indir) const
            {
                size_t l = 1;
                for(const std::pair<int,int>& el : _data)
                {
                    dest[l] = el.second;
                    if(dest[l] != 0)
                    {
                        indir[l] = el.first + 1;
                        ++l;
                    }
                }
                return l;
            }
            
            
        private:
            Vector(const std::vector<int>& data)
            {
                for(size_t i = 0; i < data.size(); ++i)
                {
                    if(data[i] != 0)
                    {
                        _data.emplace_back(i, data[i]);
                    }
                }                
            }

            std::vector<std::pair<int,int>> _data;
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
            return MurmurHash64A(k.raw(), 
                    k.data_size(), 
                    1337);
        }
    };
}

#endif /* VECTOR_H */

