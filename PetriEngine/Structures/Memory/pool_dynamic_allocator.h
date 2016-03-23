/* 
 * File:   pool_dynamic_allocator.h
 * Author: Peter G. Jensen
 *
 * Created on 23 February 2016, 22:43
 */

#ifndef POOL_DYNAMIC_ALLOCATOR_H
#define POOL_DYNAMIC_ALLOCATOR_H

#include <array>
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

namespace pdalloc
{
    struct free_t {
        free_t* next;
    };

    struct state_t {
        std::vector<char*> data;
        std::array<free_t*, 256> freelist;   
        size_t firstfree;
        size_t mem;
    };
    
    template <class T, size_t C = 1024*1024*10, size_t MINSIZE = sizeof(size_t)>
    class pool_dynamic_allocator {
        public:
            typedef T value_type;
            pool_dynamic_allocator(); // 10 MB per allocation
            ~pool_dynamic_allocator();
            void clear()
            {
                if(state.unique())
                {
                    for(auto& d : state->data)
                    {
                        PREFFREE(d);
                    }
                    state->data.clear();
                    for(auto& f : state->freelist)
                    {
                        f = NULL;
                    }
                }
                state = std::make_shared<state_t>();
                state->firstfree = 0;
                state->mem = 0;
                for(auto& f : state->freelist)
                {
                    f = NULL;
                }
            }
            template <class U> pool_dynamic_allocator(const pool_dynamic_allocator<U, C, MINSIZE>& other)
            {
                clear();
                state = other.state;
            }
            T* allocate(std::size_t n);
            void deallocate(T* p, std::size_t n);    
            template<typename U>
            struct rebind {
                typedef pool_dynamic_allocator<U, C, MINSIZE> other;
            };
            inline size_t maxsize()
            {
                return MINSIZE+255;
            }
            inline size_t memory()
            {
                return state->mem;
            }
        public:        
            std::shared_ptr<state_t> state;
    };

    template <class T, class U, size_t C, size_t MINSIZE>
    bool operator==(const pool_dynamic_allocator<T, C,MINSIZE>& a, const pool_dynamic_allocator<U, C,MINSIZE>& b)
    {
        return static_cast<void*>(a.state.get()) == static_cast<void*>(b.state.get());
    }

    template <class T, class U, size_t C, size_t MINSIZE>
    bool operator!=(const pool_dynamic_allocator<T, C,MINSIZE>& a, const pool_dynamic_allocator<U, C,MINSIZE>& b)
    {
        return !(a == b);
    }

    template <class T, size_t C, size_t MINSIZE>
    pool_dynamic_allocator<T, C,MINSIZE>::pool_dynamic_allocator()
    {
        state = std::make_shared<state_t>();
        state->firstfree = 0;
        state->mem = 0;
        for(auto& f : state->freelist)
        {
            f = NULL;
        }
        assert(C > maxsize());
        assert(MINSIZE >= sizeof(size_t));
    }

    template <class T, size_t C, size_t MINSIZE>
    pool_dynamic_allocator<T, C,MINSIZE>::~pool_dynamic_allocator()
    {
        if(state.unique())
        {
            for(auto& d : state->data)
            {
                PREFFREE(d);
            }
            state->data.clear();
            for(auto& f : state->freelist)
            {
                f = NULL;
            }
        }
    }

    template <class T, size_t C, size_t MINSIZE>
    T* pool_dynamic_allocator<T,C,MINSIZE>::allocate(std::size_t n)
    {
        state->mem += n*sizeof(T);
        T* data = NULL;
        if(n*sizeof(T) < MINSIZE || n*sizeof(T) > maxsize())
        {
            data = static_cast<T*>(PREFMALLOC(n*sizeof(T))); 
        }
        else
        {
            free_t* f = state->freelist[(n*sizeof(T))-MINSIZE];
            if(f != NULL)
            {
                data = (T*)f;
                state->freelist[(n*sizeof(T))-MINSIZE] = f->next;
            }
            else //(data == NULL)
            {
                if(state->firstfree % C == 0)
                {
                    state->data.push_back(static_cast<char*>(PREFMALLOC(C)));
                }
                else if((state->firstfree % C) + (n*sizeof(T)) > C)
                {
                    size_t size = C - (state->firstfree % C);
                    assert((state->firstfree + size) % C == 0);
                    if(size >= MINSIZE)
                    {
                        free_t* tmp = (free_t*)&(state->data[state->firstfree/C][state->firstfree%C]);
                        tmp->next = state->freelist[size-MINSIZE];
                        state->freelist[size-MINSIZE] = tmp;
                    }
                    state->firstfree = ((state->firstfree/C)+1) * C;
                    state->data.push_back(static_cast<char*>(PREFMALLOC(C)));
                }
                data = (T*)&(state->data[state->firstfree/C][state->firstfree%C]);
                state->firstfree += n*sizeof(T);
            }
        }
        assert(data != NULL);
        return data;
    }

    template <class T, size_t C, size_t MINSIZE>
    void pool_dynamic_allocator<T,C,MINSIZE>::deallocate(T* p, std::size_t n)
    {
        if(n*sizeof(T) < MINSIZE || n*sizeof(T) > maxsize())
        {
            PREFFREE(p);
        }
        else
        {
            free_t* tmp = (free_t*)p;
            tmp->next = state->freelist[(n*sizeof(T))-MINSIZE];
            state->freelist[(n*sizeof(T))-MINSIZE] = tmp;
        }
    }
}

#endif /* POOL_DYNAMIC_ALLOCATOR_H */

