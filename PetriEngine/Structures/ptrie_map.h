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
        using pt = set_stable<PTRIEDEF>;
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

