/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#include "PetriEngine/Colored/VarMultiset.h"

namespace PetriEngine::Colored {
    VarMultiset::VarMultiset(const VarMultiset::VarTuple &vars, uint32_t multiplicity) : _set(), _types(inferTypes(vars)) {
        (*this)[vars] = multiplicity;
    }

    VarMultiset::Iterator VarMultiset::begin() const {
        return Iterator(this, 0);
    }

    VarMultiset::Iterator VarMultiset::end() const {
        return Iterator(this, _set.size());
    }

    bool VarMultiset::operator==(const VarMultiset &other) const {
        VarMultiset thisMinusOther(*this);
        thisMinusOther -= other;
        VarMultiset otherMinusThis(other);
        otherMinusThis -= *this;
        return thisMinusOther.empty() && otherMinusThis.empty();
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
            (*this)[pair.first] = pair.second < other[pair.first] ? 0 : pair.second - other[pair.first];
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

    bool VarMultiset::isSubsetOrEqTo(const VarMultiset &other) const {
        VarMultiset thisMinusOther(*this);
        thisMinusOther -= other;
        return thisMinusOther.empty();
    }

    uint32_t VarMultiset::numberOfTimesThisFitsInto(const VarMultiset &other) const {
        if (_types.empty())
            return std::numeric_limits<uint32_t>::max();
        if (_types != other._types)
            throw base_error("You cannot compare variable multisets of different types");
        if (!this->isSubsetOrEqTo(other)) return 0;
        uint32_t k = std::numeric_limits<uint32_t>::max();
        for (const auto &pair : other) {
            if ((*this)[pair.first] != 0) {
                k = std::min(k, pair.second / (*this)[pair.first]);
            }
        }
        return k;
    }

    bool VarMultiset::divides(const VarMultiset &other) const {
        VarMultiset ms(*this);
        auto k = ms.numberOfTimesThisFitsInto(other);
        return (other - (*this) * k).empty();
    }

    std::string VarMultiset::toString() const {
        std::ostringstream oss;
        for (size_t i = 0; i < _set.size(); ++i) {
            auto &entry = _set[i];
            oss << _set[i].second << "'(";
            for (int j = 0; j < entry.first.size(); ++j) {
                oss << entry.first[j]->name;
                if (j < entry.first.size() - 1) {
                    oss << ", ";
                }
            }
            oss << ")";
            if (i < _set.size() - 1) {
                oss << " + ";
            }
        }
        return oss.str();
    }

    bool VarMultiset::matchesType(const VarTuple &vt) const {
        if (_types.size() != vt.size()) return false;
        for (int i = 0; i < _types.size(); ++i) {
            if (_types[i] != vt[i]->colorType) return false;
        }
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

    std::pair<const std::vector<const Variable *>, const uint32_t &> VarMultiset::Iterator::operator*() {
        return _ms->_set[_index];
    }
}