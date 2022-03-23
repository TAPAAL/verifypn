/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/VarMultiset.h"

namespace PetriEngine::Colored {
    VarMultiset::Iterator VarMultiset::begin() const {
        return Iterator(this, 0);
    }

    VarMultiset::Iterator VarMultiset::end() const {
        return Iterator(this, _set.size());
    }

    VarMultiset VarMultiset::operator+(const VarMultiset &other) const {
        VarMultiset ms(*this);
        ms += other;
        return ms;
    }

    VarMultiset VarMultiset::operator-(const VarMultiset &other) const {
        VarMultiset ms(*this);
        ms -= other;
        return ms;
    }

    VarMultiset VarMultiset::operator*(uint32_t scalar) const {
        VarMultiset ms(*this);
        ms *= scalar;
        return ms;
    }

    void VarMultiset::operator+=(const VarMultiset &other) {
        if (_types.empty()) _types = other._types;
        if (_types != other._types)
            throw base_error("You cannot compare variable multisets of different types");
        for (auto &pair: other._set) {
            (*this)[pair.first] += pair.second;
        }
    }

    void VarMultiset::operator-=(const VarMultiset &other) {
        if (_types.empty()) _types = other._types;
        if (!other._types.empty() && _types != other._types)
            throw base_error("You cannot add multisets over different sets");
        for (auto &pair: _set) {
            (*this)[pair.first] = std::min<uint32_t>(pair.second - other[pair.first], 0); // min because underflow
        }
    }

    void VarMultiset::operator*=(uint32_t scalar) {
        for (auto &pair: _set) {
            pair.second *= scalar;
        }
    }

    uint32_t VarMultiset::operator[](const VarTuple &vt) const {
        if (_types.empty()) return 0;
        assert(matchesType(vt));
        for (auto &pair: _set) {
            if (pair.first == vt) return pair.second;
        }
        return 0;
    }

    uint32_t &VarMultiset::operator[](const VarTuple &vt) {
        if (_types.empty()) _types = inferTypes(vt);
        assert(matchesType(vt));
        for (auto &pair: _set) {
            if (pair.first == vt) return pair.second;
        }
        _set.emplace_back(vt, 0);
        return _set.back().second;
    }

    size_t VarMultiset::size() const {
        size_t res = 0;
        for (auto &pair: _set) {
            res += pair.second;
        }
        return res;
    }

    bool VarMultiset::isSubsetOf(const VarMultiset &other) const {
        VarMultiset thisMinusOther(*this);
        thisMinusOther -= other;
        VarMultiset otherMinusThis(other);
        otherMinusThis -= *this;
        return thisMinusOther.empty() && !otherMinusThis.empty();
    }

    std::string VarMultiset::toString() const {
        std::ostringstream oss;
        for (size_t i = 0; i < _set.size(); ++i) {
            oss << _set[i].second << "'" << _set[i].first->name;
            if (i < _set.size() - 1) {
                oss << " + ";
            }
        }
        return oss.str();
    }

    bool VarMultiset::matchesType(const std::vector<const Variable *> &vartuple) const {
#ifndef NDEBUG
        if (_ptype != nullptr) {
            assert(vartuple.size() == _ptype->productSize());
            for (int i = 0; i < vartuple.size(); ++i) {

            }
            return true;
        }
        assert(vartuple.size() == 1);
        assert(vartuple[0]->colorType == _type);
#endif
        return true;
    }

    std::vector<const ColorType *> VarMultiset::inferTypes(const VarTuple &vt) {
        std::vector<const ColorType *> types;
        for (auto v : vt) {
            types.emplace_back(v->colorType);
        }
        return types;
    }

    bool VarMultiset::Iterator::operator==(VarMultiset::Iterator &other) {
        return _ms == other._ms && _index == other._index;
    }

    bool VarMultiset::Iterator::operator!=(VarMultiset::Iterator &other) {
        return !(*this == other);
    }

    VarMultiset::Iterator &VarMultiset::Iterator::operator++() {
        ++_index;
        return *this;
    }

    std::pair<const Variable *, const uint32_t &> VarMultiset::Iterator::operator*() {
        return _ms->_set[_index];
    }
}