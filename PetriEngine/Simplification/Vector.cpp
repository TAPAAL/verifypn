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

#include "Vector.h"
#include "LPCache.h"

namespace PetriEngine {
    namespace Simplification {
        
        const int* Vector::data() const
        {
            return _data.data();
        }

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

        size_t Vector::size() const 
        {
            return _data.size();
        }
        
    }
}