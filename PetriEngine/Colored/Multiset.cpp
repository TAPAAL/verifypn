/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Multiset.cpp
 * Author: andreas
 * 
 * Created on February 20, 2018, 10:37 AM
 */

#include "Multiset.h"

namespace PetriEngine {
    namespace Colored {
        Multiset::Multiset() {
        }

        Multiset::Multiset(const Multiset& orig) {
            _set = orig._set;
        }
        
        Multiset::Multiset(std::vector<std::pair<const Color*,uint32_t>>& colors)
                : _set(colors)
        {
        }

        Multiset::~Multiset() {
        }
        
        Multiset Multiset::operator +(const Multiset& other) const {
            Multiset ms(*this);
            ms += other;
            return ms;
        }
        
        Multiset Multiset::operator -(const Multiset& other) const {
            Multiset ms(*this);
            ms -= other;
            return ms;
        }
        
        Multiset Multiset::operator *(uint32_t scalar) const {
            Multiset ms(*this);
            ms *= scalar;
            return ms;
        }
        
        void Multiset::operator +=(const Multiset& other) {
            
        }
        
        void Multiset::operator -=(const Multiset& other) {
            
        }
        
        void Multiset::operator *=(uint32_t other) {
            
        }
    }
}

