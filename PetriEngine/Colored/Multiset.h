/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Multiset.h
 * Author: andreas
 *
 * Created on February 20, 2018, 10:37 AM
 */

#ifndef MULTISET_H
#define MULTISET_H

#include <vector>
#include <utility>

#include "Colors.h"


namespace PetriEngine {
    namespace Colored {
        class Multiset {
        public:
            Multiset();
            Multiset(const Multiset& orig);
            virtual ~Multiset();
            
            Multiset operator+ (Multiset& other);
            Multiset operator- (Multiset& other);
            Multiset operator* (uint32_t scalar);
            void operator+= (Multiset& other);
            void operator-= (Multiset& other);
            void operator*= (uint32_t scalar);
            
        private:
            std::vector<std::pair<Color*,uint32_t>> _data;
        };
    }
}

#endif /* MULTISET_H */

