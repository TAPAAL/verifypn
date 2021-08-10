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
        std::unique_ptr<T[]> _data;
    public:
        light_deque(size_t initial_size = 64)
        {
            if(initial_size == 0) initial_size = 1;
            _data.reset((T*)new uint8_t[initial_size*sizeof(T)]); // TODO, revisit with cast and initialization
            _size = initial_size;
        }

        light_deque<T> &operator=(const light_deque<T> &other) {
            _front = other._front;
            _back = other._back;
            _size = other.size();
            _data.reset((T*)new uint8_t[_size*sizeof(T)]);
            memcpy(_data.get(), other._data.get(), _size * sizeof(T));
            return *this;
        }

        light_deque(light_deque &&) noexcept = default;
        light_deque &operator=(light_deque &&) noexcept = default;

        void push_back(const T& element)
        {
            _data.get()[_back] = element;
            ++_back;
            if(_back == _size)
            {
                expand();
            }
        }

        void push_back(T&& element)
        {
            new (&_data.get()[_back]) T(std::move(element));
            ++_back;
            if(_back == _size) {
                expand();
            }
        }

        bool empty() const
        {
            return _front == _back;
        }
        
        size_t size() const
        {
            return _back - _front;
        }
        
        const T& front() const
        {
            return _data.get()[_front];
        }
        
        T& front() {
            return _data.get()[_front];
        }

        const T& back() const
        {
            return _data.get()[_back - 1];
        }
        
        T& back() {
            return _data.get()[_back - 1];
        }
        
        void pop_front()
        {
            ++_front;
            if(_front >= _back)
            {
                _front = _back = 0;
            }
        }
        
        void pop_back()
        {
            if(_back > _front)
                --_back;
            if(_back == _front)
                clear();
        }
        
        void clear()
        {
            _front = _back = 0;
        }
    private:
        void expand() {
            std::unique_ptr<T[]> ndata((T*)new uint8_t[_size*2*sizeof(T)]);
            std::move(_data.get(), _data.get() + _size, ndata.get());
            _size *= 2;
            _data.swap(ndata);
        }
};


#endif /* LIGHT_DEQUE_H */

