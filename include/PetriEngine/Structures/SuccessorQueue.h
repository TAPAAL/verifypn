/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#ifndef VERIFYPN_SUCCESSORQUEUE_H
#define VERIFYPN_SUCCESSORQUEUE_H

#include <algorithm>
#include <cassert>
#include <memory>
#include <cstdint>

class SuccessorQueue {
public:
    SuccessorQueue(uint32_t *src, uint32_t nelem)
            : _front(0), _size(nelem)
    {
        _data = std::make_unique<uint32_t[]>(nelem);
        memcpy(_data.get(), src, sizeof(uint32_t) * nelem);
    }

    SuccessorQueue() noexcept :_front(0), _size(0), _data(nullptr) {}

    [[nodiscard]] uint32_t front() const
    {
        assert(!empty());
        return _data[_front];
    }

    void pop()
    {
        assert(!empty());
        ++_front;
    }

    [[nodiscard]] uint32_t size() const
    {
        return _size - _front;
    }

    [[nodiscard]] bool empty() const
    {
        return _front >= _size;
    }

    [[nodiscard]] bool valid() const { return _data != nullptr; }

    [[nodiscard]] bool has_consumed() const { return _front > 0; }

    /**
     * Extend successor list while excluding previously popped elements.
     * @param src C-array containing the new successor list
     * @param nelem the length of src
     */
    void extend_to(uint32_t *src, uint32_t nelem)
    {
        auto newdata = std::make_unique<uint32_t[]>(nelem);
        if (_front != 0)
            memcpy(newdata.get(), _data.get(), sizeof(uint32_t) * _front);
        uint32_t sz = _front;
        for (uint32_t i = 0; i < nelem; ++i) {
            auto it = std::find(_data.get(), _data.get() + _front, src[i]);
            if (it != _data.get() + _front) {
                newdata[sz++] = src[i];
            }
        }
        _size = sz;
        assert(_size == nelem);
        _data.swap(newdata);
    }

private:
    uint32_t _front;
    uint32_t _size;
    std::unique_ptr<uint32_t[]> _data;
};

#endif //VERIFYPN_SUCCESSORQUEUE_H
