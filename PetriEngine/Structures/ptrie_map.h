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
 * File:   ptrie_stable.h
 * Author: Peter G. Jensen
 *
 *
 * Created on 06 October 2016, 13:51
 */

#ifndef PTRIE_STABLE_H
#define PTRIE_STABLE_H
#include "ptrie_stable.h"
namespace ptrie {

    template<
    typename T,
    uint16_t HEAPBOUND = 128,
    uint16_t SPLITBOUND = 128,
    size_t ALLOCSIZE = (1024 * 64),
    size_t FWDALLOC = 256,
    typename I = size_t>
    class map :
    public set_stable<HEAPBOUND, SPLITBOUND, ALLOCSIZE, FWDALLOC, T, I> {
#define pt set_stable<PTRIEDEF>
    public:
        using pt::set_stable;
        T& getData(I index);

    };

    template<
    typename T,
    uint16_t HEAPBOUND,
    uint16_t SPLITBOUND,
    size_t ALLOCSIZE,
    size_t FWDALLOC,
    typename I>
    T&
    map<T, HEAPBOUND, SPLITBOUND, ALLOCSIZE, FWDALLOC, I>::getData(I index) {
        typename pt::entry_t& ent = this->_entries->operator[](index);
        return ent.data;
    }
}

#endif /* PTRIE_STABLE_H */

