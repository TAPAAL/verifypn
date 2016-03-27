/* 
 * File:   pool_fixed_allocator.h
 * Author: Peter G. Jensen
 *
 * Created on 23 February 2016, 10:59
 */

#ifndef POOL_FIXED_ALLOCATOR_H
#define POOL_FIXED_ALLOCATOR_H

#include <cstddef>
#include <typeinfo>
#include <memory>
#include <stack>
#include <assert.h>

#ifdef JEMALLOC
extern "C"
{
    #include "jemalloc/jemalloc.h"
}
#define PREFMALLOC jemalloc_malloc
#define PREFFREE jemalloc_free
#else
#define PREFMALLOC malloc
#define PREFFREE free
#endif

namespace palloc
{
    struct free_t {
        free_t* next;
    };
    
    struct state_t {
        std::vector<char*> data;
        std::stack<char*> freestack; 
        free_t* nextfree;        
        size_t firstfree;            
        short unsigned int size;
        size_t chunksize;
    };

    template <class T, size_t C = 1024*1024*10>
    class pool_fixed_allocator {
        public:
            typedef T value_type;
            pool_fixed_allocator(short unsigned int c = sizeof(T)); // 10 MB per allocation
            ~pool_fixed_allocator();
            template <class U> pool_fixed_allocator(const pool_fixed_allocator<U, C>& other)
            : state(other.state)
            {
            }
            T* allocate(std::size_t n);
            void deallocate(T* p, std::size_t n);    
            template<typename U>
            struct rebind {
                typedef pool_fixed_allocator<U, C> other;
            };
        public:        
            std::shared_ptr<state_t> state;
    };

    template <class T, class U, size_t C>
    bool operator==(const pool_fixed_allocator<T, C>& a, const pool_fixed_allocator<U, C>& b)
    {
        return static_cast<void*>(a.state.get()) == static_cast<void*>(b.state.get());
    }

    template <class T, class U, size_t C>
    bool operator!=(const pool_fixed_allocator<T, C>& a, const pool_fixed_allocator<U, C>& b)
    {
        return !(a == b);
    }

    template <class T, size_t C>
    pool_fixed_allocator<T, C>::pool_fixed_allocator(short unsigned int c)
    {
        state = std::make_shared<state_t>();
        state->firstfree = 0;
        state->size = c;
        state->chunksize = ((C/c)+1)*c;
        state->nextfree = NULL;
    }

    template <class T, size_t C>
    pool_fixed_allocator<T, C>::~pool_fixed_allocator()
    {
        if(state.unique())
        {
            for(auto& d : state->data)
            {
                PREFFREE(d);
            }
        }
    }


    template <class T, size_t C>
    T* pool_fixed_allocator<T, C>::allocate(std::size_t n)
    {
    //    std::cout << "size: " << sizeof(T) << std::endl;
        T* data = NULL;
        if(sizeof(T) != state->size)
        {
    //        std::cout << "wrong size" << std::endl;
            data = static_cast<T*>(PREFMALLOC(n*sizeof(T)));
        }
        else if(n > 1)
        {
    //        std::cout << "more than 1" << std::endl;
            data = static_cast<T*>(PREFMALLOC(n*sizeof(T)));
        }
        else
        {
    //        std::cout << "Bingo" << std::endl;

            if(state->size >= sizeof(size_t))
            {
                free_t* n = state->nextfree;
                if(n != NULL)
                {
                    state->nextfree = n->next;
                    data = (T*)n;
                }
            }
            else
            {
                if(state->freestack.size() > 0)
                {
                    T* next = (T*)(state->freestack.top());
                    state->freestack.pop();
                    data = next;
                }
            }
            
            if(data == NULL)
            {
                if(state->firstfree % state->chunksize == 0)
                {
                    state->data.push_back((char*)(PREFMALLOC(state->chunksize)));
                }
                char* tmp = &state->data[state->firstfree/state->chunksize][state->firstfree % state->chunksize];
                T* next = (T*)(tmp);
                state->firstfree += state->size;
                data = next;
            }
        }
    //    std::cout << "Allocated " << data << std::endl;
        assert(data != NULL);
        return data;
    }

    template <class T, size_t C>
    void pool_fixed_allocator<T, C>::deallocate(T* p, std::size_t n)
    {
    //    std::cout << "Dealloc " << p << " size: " <<sizeof(T) << " n : " << n << std::endl;
        if(sizeof(T) != state->size || n != 1)
        {
            PREFFREE(p);
        }
        else
        {
            if(state->size >= sizeof(size_t))
            {
                free_t* n = (free_t*)p;
                n->next = state->nextfree;
                state->nextfree = n;
            }
            else
            {
                state->freestack.push((char *)(p));
            }
        }
    }
}


#endif /* POOL_FIXED_ALLOCATOR_H */

