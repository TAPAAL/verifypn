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
#include <set>

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

            bool isSound() {
                return _lower <= _upper;
            }

            bool contains(uint32_t id){
                return _lower <= id && id <= _upper;
            }

            void free() {
                _upper = max();
                _lower = min();
            }

            uint32_t size(){
                return 1 + _upper - _lower;
            }

            void invalidate() {
                //hack setting range invalid
                _lower = 1;
                _upper = 0;
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

        struct interval_t {
            std::vector<Reachability::range_t> _ranges;

            interval_t() {
            }

            interval_t(std::vector<Reachability::range_t> ranges) : _ranges(ranges) {
            }

            size_t size(){
                return _ranges.size();
            }

            uint32_t intervalCombinations(){
                uint32_t product = 1;
                for(auto range : _ranges){
                    product *= range.size();
                }
                if(_ranges.empty()){
                    return 0;
                }
                return product;
            }

            bool isSound(){
                for(auto range: _ranges) {
                    if(!range.isSound()){
                        return false;
                    }
                }
                return true;
            }

            void addRange(range_t newRange) {
                _ranges.push_back(newRange);
                // uint32_t vecIndex = 0;
                // std::vector<uint32_t> _rangesToRemove;
                // for (uint32_t i = 0; i < _ranges.size(); i++) {
                //     Reachability::range_t range = _ranges[i];
                //     if(newRange._lower < range._lower && newRange._upper > range._upper) {
                //         range._lower = newRange._lower;
                //         range._upper = newRange._upper;
                //         return;
                //     } else if (newRange._lower >= range._lower && newRange._lower <= range._upper) {
                //         range._upper = std::max(newRange._upper, range._upper);
                //         return;
                //     } else if (newRange._upper >= range._lower && newRange._upper <= range._upper) {
                //         range._lower = std::min(newRange._lower, range._lower);
                        
                //         if(range._lower < _ranges[i-1]._lower){
                //             _rangesToRemove.push_back(i-1);
                //         }
                //         return;
                //     } else if (newRange._upper >= range._lower-2 && newRange._lower < range._lower) {
                //         range._lower = newRange._lower;
                //         return;
                //     } else if (newRange._lower <= range._upper+2 && newRange._upper > range._upper){
                //         range._upper = newRange._upper;
                //         return;
                //     }

                //     if(newRange._lower > range._lower) {
                //         vecIndex++;
                //     }
                // }

                // for(auto index: _rangesToRemove) {
                //     _ranges.erase(_ranges.begin() + index);
                // }

                // _ranges.insert(_ranges.begin() + vecIndex, newRange);
            }

            void addRange(range_t newRange, uint32_t index){
                _ranges.insert(_ranges.begin() + index, newRange);
            }

            void addRange(uint32_t l, uint32_t u) {
                // uint32_t vecIndex = 0;
                // std::vector<uint32_t> _rangesToRemove;
                // for (uint32_t i = 0; i < _ranges.size(); i++) {
                //     Reachability::range_t range = _ranges[i];
                //     if(l < range._lower && u > range._upper) {
                //         range._lower = l;
                //         range._upper = u;
                //         return;
                //     } else if (l >= range._lower && l <= range._upper) {
                //         range._upper = std::max(u, range._upper);
                //         return;
                //     } else if (u >= range._lower && u <= range._upper) {
                //         range._lower = std::min(l, range._lower);
                        
                //         if(range._lower < _ranges[i-1]._lower){
                //             _rangesToRemove.push_back(i-1);
                //         }
                //         return;
                //     } else if (u >= range._lower-2 && l < range._lower) {
                //         range._lower = l;
                //         return;
                //     } else if (l <= range._upper+2 && u > range._upper){
                //         range._upper = u;
                //         return;
                //     }

                //     if(l > range._lower) {
                //         vecIndex++;
                //     }
                // }

                // for(auto index: _rangesToRemove) {
                //     _ranges.erase(_ranges.begin() + index);
                // }

                // _ranges.insert(_ranges.begin() + vecIndex,Reachability::range_t(l, u));
                _ranges.push_back(Reachability::range_t(l, u));
            }

            void addRange(uint32_t lower, uint32_t upper, uint32_t index){
                _ranges.insert(_ranges.begin() + index, Reachability::range_t(lower, upper));
            }

            range_t& operator[] (size_t index) {
                return _ranges[index];
            }
            
            range_t& operator[] (int index) {
                return _ranges[index];
            }
            
            range_t& operator[] (uint32_t index) {
                assert(index < _ranges.size());
                return _ranges[index];
            }

            range_t& getFirst() {
                return _ranges[0];
            }

            range_t& getLast() {
                return _ranges.back();
            }

            std::vector<uint32_t> getLowerIds(){
                std::vector<uint32_t> ids;
                for(auto range : _ranges){
                    ids.push_back(range._lower);
                }
                return ids;
            }

            std::vector<uint32_t> getUpperIds(){
                std::vector<uint32_t> ids;
                for(auto range : _ranges){
                    ids.push_back(range._upper);
                }
                return ids;
            }

            bool equals(interval_t other){
                if(other.size() != size()){
                    return false;
                }
                for(uint32_t i = 0; i < size(); i++){
                    auto comparisonRes = _ranges[i].compare(other[i]);
                    if(!comparisonRes.first || !comparisonRes.second){
                        return false;
                    }
                }
                return true;
            }

            bool constains(interval_t other){
                if(other.size() != size()){
                    return false;
                }
                for(uint32_t i = 0; i < size(); i++){
                    if(!_ranges[i].compare(other[i]).first){
                        return false;
                    }
                }
                return true;
            }

            void constrain(interval_t other){
                for(uint32_t i = 0; i < _ranges.size(); i++){
                    _ranges[i] &= other._ranges[i];
                }
            }

            void print() {
                for(auto range : _ranges){
                    std::cout << " " << range._lower << "-" << range._upper << " ";
                }
            }
        };

        struct intervalTuple_t {
            std::vector<interval_t> _intervals;

            intervalTuple_t() {
            }

            intervalTuple_t(std::vector<interval_t> ranges) :  _intervals(ranges) {
            };

            interval_t& getLower(){
                return _intervals[0];
            }

            interval_t& back() {
                return _intervals.back();
            }

            size_t size() {
                return _intervals.size();
            }

            uint32_t intervalCombinations(){
                uint32_t res = 0;
                for(auto interval : _intervals){
                    res += interval.intervalCombinations();
                }
                return res;
            }

            bool hasValidIntervals(){
                for(auto interval : _intervals) {
                    if(interval.isSound()){
                        return true;
                    }
                }
                return false;
            }

            interval_t& operator[] (size_t index) {
                return _intervals[index];
            }
            
            interval_t& operator[] (int index) {
                return _intervals[index];
            }
            
            interval_t& operator[] (uint32_t index) {
                assert(index < _intervals.size());
                return _intervals[index];
            }

            interval_t isRangeEnd(std::vector<uint32_t> ids) {
                for (uint32_t j = 0; j < _intervals.size(); j++) {
                    bool rangeEnd = true;
                    for (uint32_t i = 0; i < _intervals[j].size(); i++) {
                        auto range =  _intervals[j][i];
                        if (range._upper != ids[i]) {
                            rangeEnd = false;
                        }
                    }
                    if(rangeEnd) {
                        if(j+1 != _intervals.size()) {
                            return _intervals[j+1];
                        } else {
                            return getLower();
                        }
                    }
                }

                return interval_t();
            }

            void addInterval(interval_t interval) {
                uint32_t vecIndex = 0;

                if(!_intervals.empty()) {
                    assert(_intervals[0].size() == interval.size());
                } else {
                    _intervals.push_back(interval);
                    return;
                }

                for (auto localInterval : _intervals) {
                    bool contained = true;
                    bool foundPlace = true;

                    for(uint32_t k = 0; k < interval.size(); k++){
                        if(interval[k]._lower < localInterval[k]._lower){
                            foundPlace = true;
                            break;
                        } else if (interval[k]._lower > localInterval[k]._lower){
                            break;
                        }
                    }

                    if(foundPlace) break;

                    for (uint32_t i = 0; i < interval.size(); i++) {
                        auto range = interval[i];
                        auto constraint = localInterval[i];

                        auto result = constraint.compare(range);

                        if(!result.first) {
                            contained = false;
                            break;
                        }
                    }

                    if(contained) {
                        return;
                    }
                    vecIndex++;
                }

                _intervals.insert(_intervals.begin() + vecIndex, interval);
            }

            void constrainLower(std::vector<uint32_t> values) {
                uint32_t i = 0;
                bool done = false;
                while(!done) {
                    bool updated = false;
                    for(uint32_t j = 0; j < values.size(); j++){
                        if(_intervals[i][j]._lower <= values[j]){
                            _intervals[i][j]._lower = values[j];
                            updated = true;
                        }                        
                    }
                    done = !updated || i == _intervals.size()-1;
                    i++;
                }
                mergeIntervals();
            }

            void constrainUpper(std::vector<uint32_t> values) {
                uint32_t i = _intervals.size()-1;
                bool done = false;
                while(!done) {
                    bool updated = false;
                    for(uint32_t j = 0; j < values.size(); j++){
                        if(_intervals[i][j]._upper >= values[j]){
                            _intervals[i][j]._upper = values[j];
                            updated = true;
                        }                        
                    }

                    done = !updated || i == 0;
                    i--;
                }
                mergeIntervals();
            }

            void expandLower(std::vector<uint32_t> values) {
                for(uint32_t i = 0; i < values.size(); i++) {
                    _intervals[0][i]._lower = std::min(values[i],_intervals[0][i]._lower);
                }
            }

            void expandUpper(std::vector<uint32_t> values) {
                for(uint32_t i = 0; i < values.size(); i++) {
                    _intervals[0][i]._upper = std::max(values[i],_intervals[0][i]._upper);
                }
            }

            void print() {
                for (auto interval : _intervals){
                    std::cout << "[";
                    interval.print();
                    std::cout << "]" << std::endl;
                }
            }

            std::vector<uint32_t> getLowerIds(){
                return _intervals[0].getLowerIds();
            }

            bool contains(interval_t interval){
                for(auto localInterval : _intervals){
                    if(localInterval.constains(interval)){
                        return true;
                    }
                }
                return false;
            }

            void mergeIntervals() {
                interval_t prevConstraints;
                std::set<uint32_t> rangesToRemove;
                if(_intervals.empty()){
                    return;
                }

                for (uint32_t i = 0; i < _intervals.size(); i++) {
                    auto& interval = _intervals[i];
                    if(!interval.isSound()){
                        rangesToRemove.insert(i);
                        continue;
                    }   
                    for(uint32_t j = i+1; j < _intervals.size(); j++){
                        auto otherInterval = _intervals[j];

                        if(!otherInterval.isSound()){
                            continue;
                        }   

                        uint32_t diff = 1;
                        bool overlap = true;
                        for(uint32_t k = 0; k < interval.size(); k++) {

                            if(interval[k]._lower > otherInterval[k]._upper  ||otherInterval[k]._lower > interval[k]._upper){
                                if(interval[k]._lower > otherInterval[k]._upper +diff  ||otherInterval[k]._lower > interval[k]._upper +diff) {                                
                                    overlap = false;
                                    break;
                                } else {
                                    diff--;
                                }                                
                            }

                            // if(interval[k]._lower > otherInterval[k]._upper +1  ||otherInterval[k]._lower > interval[k]._upper +1) {
                            //     overlap = false;
                            //     break;
                            // }
                        }
                        if(overlap) {
                            for(uint32_t l = 0; l < interval.size(); l++) {
                                interval[l] |= otherInterval[l];
                            }
                            rangesToRemove.insert(j);
                        }  
                    }
                }
                for (auto i = rangesToRemove.rbegin(); i != rangesToRemove.rend(); ++i) {
                    _intervals.erase(_intervals.begin() + *i);
                }
                if(!rangesToRemove.empty()){
                    mergeIntervals();
                }
            }    
        };
    }
}


#endif /* RANGE_H */

