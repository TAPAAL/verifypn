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
            Multiset(std::pair<const Color*,uint32_t> color);
            Multiset(std::vector<std::pair<const Color*,uint32_t>>& colors);
            virtual ~Multiset();
            
            Multiset operator+ (const Multiset& other) const;
            Multiset operator- (const Multiset& other) const;
            Multiset operator* (uint32_t scalar) const;
            void operator+= (const Multiset& other);
            void operator-= (const Multiset& other);
            void operator*= (uint32_t scalar);
            
        private:
            std::vector<std::pair<const Color*,uint32_t>> _data;
        };
    }
}

#endif /* MULTISET_H */

