/*
 * File:   light_deque.h
 * Author: Peter G. Jensen <pgj@cs.aau.dk>
 *
 * Created on 03 December 2016, 12:00
 */

#ifndef LIGHT_DEQUE_H
#define LIGHT_DEQUE_H

#include <memory>
#include <cstring>

template<typename T>
class light_deque
{
    private:
        size_t _front = 0;
        size_t _back = 0;
        size_t _size = 0;
        T* _data = nullptr;
    public:
        light_deque(size_t initial_size = 64)
        {
            if(initial_size == 0) initial_size = 1;
            _data = (T*)new uint8_t[initial_size*sizeof(T)]; // TODO, revisit with cast and initialization
            _size = initial_size;
        }

        light_deque<T> &operator=(const light_deque<T> &other) {
            for(auto& e : *this)
                e.~T();
            delete[] (uint8_t*)_data;
            _front = 0;
            _back = 0;
            _size = other.size();
            _data = (T*)new uint8_t[_size*sizeof(T)];
            for(auto& e : other)
                push_back(e);
            return *this;
        }

        ~light_deque() {
            for(auto& e : *this)
            {
                e.~T();
            }
            delete[] (uint8_t*)_data;
            _data = nullptr;
        }

        inline void push_back(const T& element)
        {
            new (&_data[_back]) T(element);
            ++_back;
            if(_back == _size)
            {
                expand();
            }
        }

        inline void push_back(T&& element)
        {
            new (&_data[_back]) T(std::move(element));
            ++_back;
            if(_back == _size) {
                expand();
            }
        }

        inline bool empty() const
        {
            return _front == _back;
        }

        inline size_t size() const
        {
            return _back - _front;
        }

        inline const T& front() const
        {
            return _data[_front];
        }

        inline T& front() {
            return _data[_front];
        }

        inline const T& back() const
        {
            return _data[_back - 1];
        }

        inline T& back() {
            return _data[_back - 1];
        }

        inline void pop_front()
        {
            _data[_front].~T();
            ++_front;
            if(_front >= _back)
            {
                _front = _back = 0;
            }
        }

        inline void pop_back()
        {
            if(_back > _front)
            {
                --_back;
                _data[_back].~T();
            }
            if(_back == _front)
                clear();
        }

        inline void clear()
        {
            for(auto& e : *this)
                e.~T();
            _front = _back = 0;
        }

        inline T* begin() {
            return &front();
        }

        inline T* end() {
            return &_data[_back];
        }

        inline const T* begin() const {
            return &front();
        }

        inline const T* end() const {
            return &_data[_back];
        }

        inline const T& operator[](size_t i) const {
            return *(begin() + i);
        }

        inline T& operator[](size_t i) {
            return *(begin() + i);
        }

        private:
        void expand() {
            T* ndata = (T*)new uint8_t[_size*2*sizeof(T)];
            size_t n = 0;
            for(auto& e : *this)
            {
                new (ndata + n) T(std::move(e));
                ++n;
            }
            _size *= 2;
            _back = (_back - _front);
            _front = 0;
            std::swap(_data, ndata);
            delete[] (uint8_t*)ndata;
        }
};


#endif /* LIGHT_DEQUE_H */

