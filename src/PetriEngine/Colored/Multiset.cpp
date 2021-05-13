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
        Multiset::Multiset() : _set(), type(nullptr) {
        }

        Multiset::Multiset(const Multiset& orig) {
            _set = orig._set;
            type = orig.type;
        }

        Multiset::Multiset(std::vector<std::pair<const Color*,uint32_t>>& colors)
                : _set(), type(nullptr)
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
            if (type == nullptr) {
                type = other.type;
            }
            if (other.type != nullptr && type->getId() != other.type->getId()) {
                throw "You cannot add Multisets over different sets";
            }
            for (auto c : other._set) {
                const Color* color = DotConstant::dotConstant(nullptr);
                if (type != nullptr)
                    color = &((*type)[c.first]);
                (*this)[color] += c.second;
            }
        }

        void Multiset::operator -=(const Multiset& other) {
            if (type == nullptr) {
                type = other.type;
            }
            if (other.type != nullptr && type->getId() != other.type->getId()) {
                throw "You cannot add Multisets over different sets";
            }
            for (auto c : _set) {
                const Color* color = DotConstant::dotConstant(nullptr);
                if (type != nullptr)
                    color = &((*type)[c.first]);
                (*this)[color] = std::min(c.second - other[color], c.second);
            }
        }

        void Multiset::operator *=(uint32_t scalar) {
            for (auto& c : _set) {
                c.second *= scalar;
            }
        }

        uint32_t Multiset::operator [](const Color* color) const {
            if (type != nullptr && type->getId() == color->getColorType()->getId()) {
                for (auto c : _set) {
                    if (c.first == color->getId())
                        return c.second;
                }
            } else if (type == nullptr){
                for (auto c : _set) {
                    if (c.first == color->getId())
                        return c.second;
                }
            }

            return 0;
        }

        uint32_t& Multiset::operator [](const Color* color) {
            if (type == nullptr) {
                type = color->getColorType();
            }
            if (color->getColorType() != nullptr && type->getId() != color->getColorType()->getId()) {
                throw "You cannot access a Multiset with a color from a different color type";
            }
            for (auto & i : _set) {
                if (i.first == color->getId())
                    return i.second;
            }

            _set.emplace_back(color->getId(), 0);
            return _set.back().second;
        }

        bool Multiset::empty() const {
            return _set.empty();
        }

        void Multiset::clean() {
            if (std::find_if(_set.begin(), _set.end(), [&](auto elem) { return elem.second == 0; }) == _set.end())
                return;

            _set.erase(std::remove_if(_set.begin(), _set.end(), [&](auto elem) {
                return elem.second == 0;
            }));
        }

        Multiset::Iterator Multiset::begin() {
            return Iterator(this, 0);
        }

        Multiset::Iterator Multiset::end() {
            return Iterator(this, _set.size());
        }


        /** Multiset iterator implementation */
        bool Multiset::Iterator::operator==(Multiset::Iterator &other) {
            return ms == other.ms && index == other.index;
        }

        bool Multiset::Iterator::operator!=(Multiset::Iterator &other) {
            return !(*this == other);
        }

        Multiset::Iterator &Multiset::Iterator::operator++() {
            ++index;
            return *this;
        }

        std::pair<const Color *, uint32_t &> Multiset::Iterator::operator++(int) {
            std::pair<const Color*, uint32_t&> old = **this;
            ++index;
            return old;
        }

        std::pair<const Color *, uint32_t &> Multiset::Iterator::operator*() {
            auto& item = ms->_set[index];
            auto color = DotConstant::dotConstant(nullptr);
            if (ms->type != nullptr)
                color = &(*ms->type)[item.first];
            return { color, item.second };
        }

        std::string Multiset::toString() const {
            std::ostringstream oss;
            for (size_t i = 0; i < _set.size(); ++i) {
                oss << _set[i].second << "'(" << (*type)[_set[i].first].toString() << ")";
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

