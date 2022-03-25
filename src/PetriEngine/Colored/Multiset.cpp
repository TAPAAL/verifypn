/* Copyright (C) 2020  Andreas H. Klostergaard
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

#include <ios>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "PetriEngine/Colored/Multiset.h"

namespace PetriEngine {
    namespace Colored {
        Multiset::Multiset() : _set(), _type(nullptr) {
        }

        Multiset::Multiset(std::vector<std::pair<const Color*,uint32_t>>& colors)
                : _set(), _type(nullptr)
        {
            for (auto& c : colors) {
                (*this)[c.first] += c.second;
            }
        }

        Multiset::~Multiset() = default;

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
            if (_type == nullptr) {
                _type = other._type;
            }
            if (other._type != nullptr && _type != other._type) {
                throw "You cannot add Multisets over different sets";
            }
            for (auto c : other._set) {
                const Color* color = &(*ColorType::dotInstance()->begin());
                if (_type != nullptr)
                    color = &((*_type)[c.first]);
                (*this)[color] += c.second;
            }
        }

        void Multiset::operator -=(const Multiset& other) {
            if (_type == nullptr) {
                _type = other._type;
            }
            if (other._type != nullptr && _type != other._type) {
                throw "You cannot add Multisets over different sets";
            }
            for (auto c : _set) {
                const Color* color = &(*ColorType::dotInstance()->begin());
                if (_type != nullptr)
                    color = &((*_type)[c.first]);
                (*this)[color] = std::min(c.second - other[color], c.second);
            }
        }

        void Multiset::operator *=(uint32_t scalar) {
            for (auto& c : _set) {
                c.second *= scalar;
            }
        }

        uint32_t Multiset::operator [](const Color* color) const {
            if (_type != nullptr && _type == color->getColorType()) {
                for (auto c : _set) {
                    if (c.first == color->getId())
                        return c.second;
                }
            } else if (_type == nullptr){
                for (auto c : _set) {
                    if (c.first == color->getId())
                        return c.second;
                }
            }

            return 0;
        }

        uint32_t& Multiset::operator [](const Color* color) {
            if (_type == nullptr) {
                _type = color->getColorType();
            }
            if (color->getColorType() != nullptr && _type != color->getColorType()) {
                throw "You cannot access a Multiset with a color from a different color type";
            }
            for (auto & i : _set) {
                if (i.first == color->getId())
                    return i.second;
            }

            _set.emplace_back(color->getId(), 0);
            return _set.back().second;
        }

        bool Multiset::isSubsetOf(const Multiset &other) const {
            Multiset thisMinusOther(*this);
            thisMinusOther -= other;
            Multiset otherMinusThis(other);
            otherMinusThis -= *this;
            return thisMinusOther.empty() && !otherMinusThis.empty();
        }

        bool Multiset::isSubsetOrEqTo(const Multiset &other) const {
            Multiset thisMinusOther(*this);
            thisMinusOther -= other;
            return thisMinusOther.empty();
        }

        bool Multiset::empty() const {
            for (auto & e : _set) {
                if (e.second > 0) return false;
            }
            return true;
        }

        const Multiset::Iterator Multiset::begin() const {
            return Iterator(this, 0);
        }

        const Multiset::Iterator Multiset::end() const{
            return Iterator(this, _set.size());
        }


        /** Multiset iterator implementation */
        bool Multiset::Iterator::operator==(Multiset::Iterator &other) {
            return _ms == other._ms && _index == other._index;
        }

        bool Multiset::Iterator::operator!=(Multiset::Iterator &other) {
            return !(*this == other);
        }

        Multiset::Iterator &Multiset::Iterator::operator++() {
            ++_index;
            return *this;
        }

        std::pair<const Color *, const uint32_t &> Multiset::Iterator::operator++(int) {
            std::pair<const Color*, const uint32_t&> old = **this;
            ++_index;
            return old;
        }

        std::pair<const Color *, const uint32_t &> Multiset::Iterator::operator*() {
            auto& item = _ms->_set[_index];
            auto color = &(*ColorType::dotInstance()->begin());
            if (_ms->_type != nullptr)
                color = &(*_ms->_type)[item.first];
            return { color, item.second };
        }

        std::string Multiset::toString() const {
            std::ostringstream oss;
            for (size_t i = 0; i < _set.size(); ++i) {
                oss << _set[i].second << "'(" << (*_type)[_set[i].first].toString() << ")";
                if (i < _set.size() - 1) {
                    oss << " + ";
                }
            }

            return oss.str();
        }

        size_t Multiset::size() const {
            size_t res = 0;
            for (auto item : _set) {
                res += item.second;
            }
            return res;
        }
    }
}

