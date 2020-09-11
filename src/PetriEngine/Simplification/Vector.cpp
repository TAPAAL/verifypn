/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Vector.cpp
 * Author: Peter G. Jensen
 *
 * Created on 31 May 2017, 09:26
 */

#include "PetriEngine/Simplification/Vector.h"
#include "PetriEngine/Simplification/LPCache.h"

namespace PetriEngine {
    namespace Simplification {
        
        void Vector::free()
        {
            --ref;
            if(ref == 0)
            {
                factory->invalidate(*this);
            }
        }
        
        void Vector::inc()
        {
            ++ref;
        }

        size_t Vector::data_size() const 
        {
            return _data.size() * sizeof(std::pair<int,int>);
        }
        
    }
}
