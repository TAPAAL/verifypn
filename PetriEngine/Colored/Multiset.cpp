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

#include <ios>
#include <algorithm>

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
            for (auto c : other._set) {
                (*this)[c.first] += c.second;
            }
        }
        
        void Multiset::operator -=(const Multiset& other) {
            for (auto c : _set) {
                c.second = std::min(c.second - other[c.first], 0u);
            }
        }
        
        void Multiset::operator *=(uint32_t scalar) {
            for (auto c : _set) {
                c.second *= scalar;
            }
        }
        
        uint32_t Multiset::operator [](const Color* color) const {
            for (auto c : _set) {
                if (c.first == color)
                    return c.second;
            }
            
            return 0;
        }
        
        uint32_t& Multiset::operator [](const Color* color) {
            for (size_t i = 0; i < _set.size(); ++i) {
                if (_set[i].first == color)
                    return _set[i].second;
            }
            
            _set.push_back(std::pair<const Color*, uint32_t>(color, 0));
            return (*_set.rbegin()).second;
        }
        
        bool Multiset::empty() const {
            return false;
        }
        
        Multiset::Internal::iterator Multiset::begin() {
            return _set.begin();
        }
        
        Multiset::Internal::iterator Multiset::end() {
            return _set.end();
        }
    }
}

