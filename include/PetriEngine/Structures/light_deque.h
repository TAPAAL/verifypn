/* 
 * File:   light_deque.h
 * Author: Peter G. Jensen <pgj@cs.aau.dk>
 *
 * Created on 03 December 2016, 12:00
 */

#ifndef LIGHT_DEQUE_H
#define LIGHT_DEQUE_H

#include <memory>

using namespace std;

template<typename T>
class light_deque
{
    private:
        size_t _front = 0;
        size_t _back = 0;
        size_t _size = 0;
        unique_ptr<T[]> _data;
    public:
        light_deque(size_t initial_size = 64)
        {
            if(initial_size == 0) initial_size = 1;
            _data.reset(new T[initial_size]);
            _size = initial_size;
        }

        light_deque<T> &operator=(const light_deque<T> &other) {
            _front = other._front;
            _back = other._back;
            _size = other.size();
            _data.reset(new T[_size]);
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
        void push_back(T&& element) {
            _data.get()[_back] = element;
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
        
        T front() const
        {
            return _data.get()[_front];
        }
        const T& front() {
            return _data.get()[_front];
        }

        void pop_front()
        {
            ++_front;
            if(_front >= _back)
            {
                _front = _back = 0;
            }
        }
        
        void clear()
        {
            _front = _back = 0;
        }
    private:
        void expand() {
            unique_ptr<T[]> ndata(new T[_size*2]);
            memcpy(ndata.get(), _data.get(), _size*sizeof(T));
            _size *= 2;
            _data.swap(ndata);
        }
};


#endif /* LIGHT_DEQUE_H */

