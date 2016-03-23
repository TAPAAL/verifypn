/* 
 * File:   jemallocator.h
 * Author: Peter G. Jensen
 *
 * Created on 26 February 2016, 14:24
 */

#ifndef JEMALLOCATOR_H
#define JEMALLOCATOR_H
#ifdef JEMALLOC
extern "C"
{
    #include "jemalloc/jemalloc.h"
}
#include <limits>
template <class T>
    class jemallocator {
        public:
            typedef T value_type;
            jemallocator(){}; // 10 MB per allocation
            ~jemallocator(){};
            template <class U> jemallocator(const jemallocator<U>& other)
            {
            }
            T* allocate(std::size_t n)
            {
                return (T*)jemalloc_malloc(sizeof(T)*n);
            }
            void deallocate(T* p, std::size_t n)
            {
                jemalloc_free((void*)p);
            }
            template<typename U>
            struct rebind {
                typedef jemallocator<U> other;
            };
            inline size_t maxsize()
            {
                return std::numeric_limits<size_t>();
            }
    };
#endif
#endif /* JEMALLOCATOR_H */

