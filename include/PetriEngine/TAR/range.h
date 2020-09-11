/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   range.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 4:32 PM
 */

#ifndef RANGE_H
#define RANGE_H

#include <cinttypes>
#include <cassert>
#include <limits>
#include <iostream>
#include <vector>

namespace PetriEngine {
    namespace Reachability {

        struct range_t {

            static inline uint32_t min() {
                return std::numeric_limits<uint32_t>::min();
            }

            static inline uint32_t max() {
                return std::numeric_limits<uint32_t>::max();
            }

            range_t() {
            };

            explicit range_t(uint32_t val) : _lower(val), _upper(val) {
            }

            range_t(uint32_t l, uint32_t u) : _lower(l), _upper(u) {
                assert(_lower <= _upper);
            }

            uint32_t _lower = min();
            uint32_t _upper = max();

            bool no_upper() const {
                return _upper == max();
            }

            bool no_lower() const {
                return _lower == min();
            }

            bool unbound() const {
                return no_lower() && no_upper();
            }

            void free() {
                _upper = max();
                _lower = min();
            }

            std::ostream& print(std::ostream& os) const {
                if (no_lower())
                    os << "[0";
                else
                    os << "[" << _lower;
                os << ", ";
                if (no_upper())
                    os << "inf]";
                else
                    os << _upper << "]";
                return os;
            }

            std::pair<bool, bool> compare(const range_t& other) const {
                return std::make_pair(
                        _lower <= other._lower && _upper >= other._upper,
                        _lower >= other._lower && _upper <= other._upper
                        );
            }

            range_t& operator&=(const range_t& other) {
                _lower = std::max(_lower, other._lower);
                _upper = std::min(_upper, other._upper);
                return *this;
            }

            range_t& operator|=(const range_t& other) {
                _lower = std::min(_lower, other._lower);
                _upper = std::max(_upper, other._upper);
                return *this;
            }

            range_t& operator|=(uint32_t val) {
                _lower = std::min(val, _lower);
                _upper = std::max(val, _upper);
                return *this;
            }

            range_t& operator&=(uint32_t val) {
                _lower = val;
                _upper = val;
                return *this;
            }

            range_t& operator-=(uint32_t val) {
                if (_lower < min() + val)
                    _lower = min();
                else
                    _lower -= val;
                if (_upper != max()) {
                    if (_upper < min() + val) {
                        assert(false);
                    } else {
                        _upper -= val;
                    }
                }
                return *this;
            }

            range_t& operator+=(uint32_t val) {
                if (_lower != min()) {
                    if (_lower >= max() - val)
                        assert(false);
                    _lower += val;
                }

                if (_upper >= max() - val) {
                    _upper = max();
                } else
                    _upper += val;
                return *this;
            }
        };

        struct placerange_t {
            range_t _range;
            uint32_t _place = std::numeric_limits<uint32_t>::max();

            placerange_t() {
            }

            placerange_t(uint32_t place) : _place(place) {
            };

            placerange_t(uint32_t place, const range_t& r) : _range(r), _place(place) {
            };

            placerange_t(uint32_t place, range_t&& r) : _range(r), _place(place) {
            };

            placerange_t(uint32_t place, uint32_t v) : _range(v), _place(place) {
            };

            placerange_t(uint32_t place, uint32_t l, uint32_t u) : _range(l, u), _place(place) {
            };

            std::ostream& print(std::ostream& os) const {
                os << "<P" << _place << "> in ";
                return _range.print(os);
            }

            std::pair<bool, bool> compare(const range_t& other) const {
                return _range.compare(other);
            }

            std::pair<bool, bool> compare(const placerange_t& other) const {
                assert(other._place == _place);
                if (other._place != _place)
                    return std::make_pair(false, false);
                return _range.compare(other._range);
            }

            placerange_t& operator|=(uint32_t val) {
                _range |= val;
                return *this;
            }

            placerange_t& operator&=(uint32_t val) {
                _range &= val;
                return *this;
            }

            placerange_t& operator-=(uint32_t val) {
                _range -= val;
                return *this;
            }

            placerange_t& operator+=(uint32_t val) {
                _range += val;
                return *this;
            }

            placerange_t& operator&=(const placerange_t& other) {
                assert(other._place == _place);
                _range &= other._range;
                return *this;
            }

            placerange_t& operator|=(const placerange_t& other) {
                assert(other._place == _place);
                _range |= other._range;
                return *this;
            }

            // used for sorting only!

            bool operator<(const placerange_t& other) const {
                return _place < other._place;
            }
        };

        struct prvector_t {
            std::vector<placerange_t> _ranges;

            const placerange_t* operator[](uint32_t place) const {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    return nullptr;
                } else {
                    return &(*lb);
                }
            }

            placerange_t* operator[](uint32_t place) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    return nullptr;
                } else {
                    return &(*lb);
                }
            }

            placerange_t& find_or_add(uint32_t place) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    lb = _ranges.emplace(lb, place);
                }
                return *lb;
            }

            uint32_t lower(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return range_t::min();
                return pr->_range._lower;
            }

            uint32_t upper(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return range_t::max();
                return pr->_range._upper;
            }

            bool unbound(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return true;
                return pr->_range.unbound();
            }

            void copy(const prvector_t& other) {
                _ranges = other._ranges;
                compact();
            }

            void compact() {
                for (int64_t i = _ranges.size() - 1; i >= 0; --i) {
                    if (_ranges[i]._range.unbound())
                        _ranges.erase(_ranges.begin() + i);
                }
                assert(is_compact());
            }

            bool is_compact() const {
                for (auto& e : _ranges)
                    if (e._range.unbound())
                        return false;
                return true;
            }

            void compress() {
                int64_t i = _ranges.size();
                for (--i; i >= 0; --i)
                    if (_ranges[i]._range.unbound())
                        _ranges.erase(_ranges.begin() + i);
            }

            std::pair<bool, bool> compare(const prvector_t& other) const {
                assert(is_compact());
                assert(other.is_compact());
                auto sit = _ranges.begin();
                auto oit = other._ranges.begin();
                std::pair<bool, bool> incl = std::make_pair(true, true);

                while (true) {
                    if (sit == _ranges.end()) {
                        incl.second = incl.second && (oit == other._ranges.end());
                        break;
                    } else if (oit == other._ranges.end()) {
                        incl.first = false;
                        break;
                    } else if (sit->_place == oit->_place) {
                        auto r = sit->compare(*oit);
                        incl.first = incl.first && r.first;
                        incl.second &= incl.second && r.second;
                        ++sit;
                        ++oit;
                    } else if (sit->_place < oit->_place) {
                        incl.first = false;
                        ++sit;
                    } else if (sit->_place > oit->_place) {
                        incl.second = false;
                        ++oit;
                    }
                    if (!incl.first && !incl.second)
                        return incl;
                }
                return incl;
            }

            std::ostream& print(std::ostream& os) const {

                os << "{\n";
                for (auto& pr : _ranges) {
                    os << "\t";
                    pr.print(os) << "\n";
                }
                os << "}\n";
                return os;
            }

            bool is_true() const {
                return _ranges.empty();
            }

            bool is_false(size_t nplaces) const {
                if (_ranges.size() != nplaces) return false;
                for (auto& p : _ranges) {
                    if (p._range._lower != 0 ||
                            p._range._upper != 0)
                        return false;
                }
                return true;
            }

            bool operator<(const prvector_t& other) const {
                if (_ranges.size() != other._ranges.size())
                    return _ranges.size() < other._ranges.size();
                for (size_t i = 0; i < _ranges.size(); ++i) {
                    auto& r = _ranges[i];
                    auto& otr = other._ranges[i];

                    if (r._place != otr._place)
                        return r._place < otr._place;
                    if (r._range._lower != otr._range._lower)
                        return r._range._lower < otr._range._lower;
                    if (r._range._upper != otr._range._upper)
                        return r._range._upper < otr._range._upper;
                }
                return false;
            }

            bool operator==(const prvector_t& other) const {
                auto r = compare(other);
                return r.first && r.second;
            }

            prvector_t& operator&=(const placerange_t& other) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), other);
                if (lb == std::end(_ranges) || lb->_place != other._place) {
                    _ranges.insert(lb, other);
                } else {
                    *lb &= other;
                }
                return *this;
            }

            prvector_t& operator&=(const prvector_t& other) {
                auto oit = other._ranges.begin();
                auto sit = _ranges.begin();
                while (sit != _ranges.end()) {
                    while (oit != other._ranges.end() && oit->_place < sit->_place) {
                        sit = _ranges.insert(sit, *oit);
                        ++sit;
                        ++oit;
                    }
                    if (oit == other._ranges.end() || oit->_place != sit->_place) {
                        ++sit;
                    } else {
                        *sit &= *oit;
                        ++sit;
                    }
                }
                if (oit != other._ranges.end()) {
                    _ranges.insert(_ranges.end(), oit, other._ranges.end());
                }
                return *this;
            }

            bool restricts(const std::vector<int64_t>& writes) const {
                auto rit = _ranges.begin();
                for (auto p : writes) {
                    while (rit != std::end(_ranges) &&
                            (rit->_place < p || rit->_range.unbound()))
                        ++rit;
                    if (rit == std::end(_ranges)) break;
                    if (rit->_place == p)
                        return true;
                }
                return false;
            }
        };
    }
}


#endif /* RANGE_H */

