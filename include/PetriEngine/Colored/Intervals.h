/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
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

#ifndef INTERVALS_H
#define INTERVALS_H

#include "../TAR/range.h"
#include <set>
#include <unordered_map>
#include <chrono>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>


namespace PetriEngine {
    namespace Colored {

        struct interval_t {
            std::vector<Reachability::range_t> _ranges;

            interval_t() {
            }

            ~interval_t(){}

            interval_t(const std::vector<Reachability::range_t>& ranges) : _ranges(ranges) {
            }

            size_t size() const {
                return _ranges.size();
            }

            uint32_t intervalCombinations() const {
                uint32_t product = 1;
                for(auto range : _ranges){
                    product *= range.size();
                }
                if(_ranges.empty()){
                    return 0;
                }
                return product;
            }

            bool isSound() const {
                for(auto range: _ranges) {
                    if(!range.isSound()){
                        return false;
                    }
                }
                return true;
            }

            void addRange(Reachability::range_t&& newRange) {
                _ranges.emplace_back(newRange);
            }
            
            void addRange(const Reachability::range_t& newRange) {
                _ranges.emplace_back(newRange);
            }

            void addRange(Reachability::range_t newRange, uint32_t index){
                _ranges.insert(_ranges.begin() + index, newRange);
            }

            void addRange(uint32_t l, uint32_t u) {
                _ranges.emplace_back(l, u);
            }

            void addRange(uint32_t lower, uint32_t upper, uint32_t index){
                _ranges.insert(_ranges.begin() + index, Reachability::range_t(lower, upper));
            }

            Reachability::range_t& operator[] (size_t index) {
                assert(index < _ranges.size());
                return _ranges[index];
            }
            
            const Reachability::range_t& operator[] (size_t index) const {

                return _ranges[index];
            }


            std::vector<uint32_t> getLowerIds() const {
                std::vector<uint32_t> ids;
                for(auto& range : _ranges){
                    ids.push_back(range._lower);
                }
                return ids;
            }

            std::vector<uint32_t> getUpperIds() const {
                std::vector<uint32_t> ids;
                for(auto& range : _ranges){
                    ids.push_back(range._upper);
                }
                return ids;
            }

            interval_t getSingleColorInterval() const {
                interval_t newInterval;
                for(auto& range : _ranges){
                    newInterval.addRange(range._lower, range._lower);
                }
                return newInterval;
            }

            bool equals(const interval_t& other) const {
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

            uint32_t getContainedColors() const {
                uint32_t colors = 1;
                for(auto range : _ranges) {
                    colors *= 1+  range._upper - range._lower;
                }          
                return colors;
            }

            bool contains(const interval_t& other) const {
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

            bool contains(const interval_t &other, const std::vector<bool> &diagonalPositions) const {
                if(other.size() != size()){
                    return false;
                }
                for(uint32_t i = 0; i < size(); i++){
                    if(!diagonalPositions[i] && !_ranges[i].compare(other[i]).first){
                        return false;
                    }
                }
                return true;
            }

            void constrain(const interval_t& other) {
                for(uint32_t i = 0; i < _ranges.size(); i++){
                    _ranges[i] &= other._ranges[i];
                }
            }

            interval_t getOverlap(const interval_t &other) const {
                interval_t overlapInterval;
                if(size() != other.size()){
                    return overlapInterval;
                }

                for(uint32_t i = 0; i < size(); i++){
                    auto rangeCopy = _ranges[i];
                    overlapInterval.addRange(rangeCopy &= other[i]);
                }

                return overlapInterval;
            }

            interval_t getOverlap(const interval_t& other, const std::vector<bool> &diagonalPositions){
                interval_t overlapInterval;
                if(size() != other.size()){
                    return overlapInterval;
                }

                for(uint32_t i = 0; i < size(); i++){
                    if(diagonalPositions[i]){
                        overlapInterval.addRange(_ranges[i]);
                    } else {
                        auto rangeCopy = _ranges[i];
                        overlapInterval.addRange(rangeCopy &= other[i]);
                    }                    
                }

                return overlapInterval;
            }

            std::vector<interval_t> getSubtracted(const interval_t& other, const std::vector<bool> &diagonalPositions,  uint32_t ctSize){
                std::vector<interval_t> result;
                
                if(size() != other.size()){
                    return result;
                }
                
                for(uint32_t i = 0; i < size(); i++){
                    interval_t newInterval = *this;
                    if(diagonalPositions[i]){
                        continue;
                    }
                    
                    int32_t newMinUpper = std::min(((int) other[i]._lower) -1, (int)_ranges[i]._upper);
                    //uint32_t otherUpper = (other[i]._upper +1) >= ctSize? ctSize-1: other[i]._upper +1;
                    uint32_t newMaxLower = std::max(other[i]._upper +1, _ranges[i]._lower);

                    if(((int32_t) _ranges[i]._lower) <= newMinUpper && newMaxLower <= _ranges[i]._upper){
                        auto intervalCopy = *this;
                        auto lowerRange = Reachability::range_t(_ranges[i]._lower, newMinUpper);
                        auto upperRange = Reachability::range_t(newMaxLower, _ranges[i]._upper);
                        newInterval._ranges[i] = lowerRange;
                        intervalCopy._ranges[i] = upperRange;
                        result.push_back(std::move(intervalCopy));
                        result.push_back(std::move(newInterval));
                        
                    } else if (((int32_t) _ranges[i]._lower)  <= newMinUpper){
                        auto lowerRange = Reachability::range_t(_ranges[i]._lower, newMinUpper);
                        newInterval._ranges[i] = lowerRange;
                        result.push_back(std::move(newInterval));
                    } else if (newMaxLower <= _ranges[i]._upper) {
                        auto upperRange = Reachability::range_t(newMaxLower, _ranges[i]._upper);
                        newInterval._ranges[i] = upperRange;
                        result.push_back(std::move(newInterval));
                    }                    
                }                
                return result;
            }

            void print() const {
                for(auto range : _ranges){
                    std::cout << " " << range._lower << "-" << range._upper << " ";
                }
            }

            std::string toString() const {
                std::ostringstream strs;
                for(auto range : _ranges){
                    strs << " " << range._lower << "-" << range._upper << " ";
                }
                return strs.str();
            }
        };

        struct closestIntervals {
            uint32_t intervalId1;
            uint32_t intervalId2;
            uint32_t distance;
        };

        struct intervalTuple_t {
            std::vector<interval_t> _intervals;
            double totalinputtime = 0;

            ~intervalTuple_t() {
            }

            intervalTuple_t() {
            }

            intervalTuple_t(const std::vector<interval_t>& ranges) :  _intervals(ranges) {
            };

            const interval_t& front() const{
                return _intervals[0];
            }

            const interval_t& back() const {
                return _intervals.back();
            }

            size_t size() const {
                return _intervals.size();
            }

            size_t tupleSize() const {
                return _intervals[0].size();
            }

            uint32_t getContainedColors() const {
                uint32_t colors = 0;
                for (auto interval : _intervals) {
                    colors += interval.getContainedColors();              
                }
                return colors;
            }

            static std::pair<uint32_t,uint32_t> shiftInterval(uint32_t lower, uint32_t upper, uint32_t ctSize, int32_t modifier) {
                int32_t lower_val = ctSize + (lower + modifier);
                int32_t upper_val = ctSize + (upper + modifier);
                return std::make_pair(lower_val % ctSize, upper_val % ctSize);
            }

            uint32_t intervalCombinations() const {
                uint32_t res = 0;
                for(auto interval : _intervals){
                    res += interval.intervalCombinations();
                }
                return res;
            }

            bool hasValidIntervals() const {
                for(auto interval : _intervals) {
                    if(interval.isSound()){
                        return true;
                    }
                }
                return false;
            }

            const interval_t& operator[] (size_t index) const {
                assert(index < _intervals.size());
                return _intervals[index];
            }

            interval_t& operator[] (size_t index) {
                assert(index < _intervals.size());
                return _intervals[index];
            }
            
            interval_t isRangeEnd(const std::vector<uint32_t>& ids) const {
                for (uint32_t j = 0; j < _intervals.size(); j++) {
                    bool rangeEnd = true;
                    for (uint32_t i = 0; i < _intervals[j].size(); i++) {
                        auto range =  _intervals[j][i];
                        if (range._upper != ids[i]) {
                            rangeEnd = false;
                            break;
                        }
                    }
                    if(rangeEnd) {
                        if(j+1 != _intervals.size()) {
                            return _intervals[j+1];
                        } else {
                            return front();
                        }
                    }
                }
                return interval_t();
            }

            std::vector<Colored::interval_t> shrinkIntervals(uint32_t newSize) const {
                std::vector<Colored::interval_t> resizedIntervals;
                for(auto& interval : _intervals){
                    Colored::interval_t resizedInterval;
                    for(uint32_t i = 0; i < newSize; i++){
                        resizedInterval.addRange(interval[i]);
                    }
                    resizedIntervals.push_back(resizedInterval);
                }
                return resizedIntervals;
            }

            void addInterval(const interval_t& interval) {
                uint32_t vecIndex = 0;

                if(!_intervals.empty()) {
                    assert(_intervals[0].size() == interval.size());
                } else {
                    _intervals.push_back(interval);
                    return;
                }

                for (auto& localInterval : _intervals) {
                    bool extendsInterval = true;
                    enum FoundPlace {undecided, greater, lower};
                    FoundPlace foundPlace = undecided;

                    for(uint32_t k = 0; k < interval.size(); k++){
                        if(interval[k]._lower > localInterval[k]._upper  || localInterval[k]._lower > interval[k]._upper){
                            extendsInterval = false;
                        }
                        if(interval[k]._lower < localInterval[k]._lower){
                            if(foundPlace == undecided){
                                foundPlace = lower;
                            }                            
                        } else if (interval[k]._lower > localInterval[k]._lower){
                            if(foundPlace == undecided){
                                foundPlace = greater;
                            }                            
                        }
                        if(!extendsInterval && foundPlace != undecided){
                            break;
                        }
                    }

                    if(extendsInterval) {
                        for(uint32_t k = 0; k < interval.size(); k++){
                            localInterval[k] |= interval[k];
                        }
                        return;
                    } else if(foundPlace == lower){
                        break;
                    }                    
                    vecIndex++;
                }

                _intervals.insert(_intervals.begin() + vecIndex, interval);
            }

            void constrainLower(const std::vector<uint32_t>& values, bool strict) {
                for(uint32_t i = 0; i < _intervals.size(); ++i) {
                    for(uint32_t j = 0; j < values.size(); ++j){
                        if(strict && _intervals[i][j]._lower <= values[j]){
                            _intervals[i][j]._lower = values[j]+1;
                        } 
                        else if(!strict && _intervals[i][j]._lower < values[j]){
                            _intervals[i][j]._lower = values[j];
                        }                          
                    }
                }
                simplify();
            }

            void constrainUpper(const std::vector<uint32_t>& values, bool strict) {
                for(uint32_t i = 0; i < _intervals.size(); ++i) {
                    for(uint32_t j = 0; j < values.size(); ++j){
                        if(strict && _intervals[i][j]._upper >= values[j]){
                            _intervals[i][j]._upper = values[j]-1;
                        } 
                        else if(!strict && _intervals[i][j]._upper > values[j]){
                            _intervals[i][j]._upper = values[j];
                        }                        
                    }
                }
                simplify();
            }

            void expandLower(const std::vector<uint32_t>& values) {
                for(uint32_t i = 0; i < values.size(); i++) {
                    _intervals[0][i]._lower = std::min(values[i],_intervals[0][i]._lower);
                }
            }

            void expandUpper(const std::vector<uint32_t>& values) {
                for(uint32_t i = 0; i < values.size(); i++) {
                    _intervals[0][i]._upper = std::max(values[i],_intervals[0][i]._upper);
                }
            }

            void print() const {
                for (auto interval : _intervals){
                    std::cout << "[";
                    interval.print();
                    std::cout << "]" << std::endl;
                }
            }

            std::string toString() const {
                std::string out;
                for (auto interval : _intervals){
                    out += "[";
                    out += interval.toString();
                    out += "]\n";
                }
                return out;
            }

            std::vector<uint32_t> getLowerIds() const {
                std::vector<uint32_t> ids;
                for(const auto& interval : _intervals){
                    if(ids.empty()){
                        ids = interval.getLowerIds();
                    } else {
                        for(uint32_t i = 0; i < ids.size(); i++){
                            ids[i] = std::min(ids[i], interval[i]._lower);
                        }
                    }
                }
                return ids;
            }

            std::vector<uint32_t> getLowerIds(int32_t modifier, const std::vector<size_t>& sizes) const {
                std::vector<uint32_t> ids;
                for(uint32_t j = 0; j < size(); j++){
                    auto& interval = _intervals[j];
                    if(ids.empty()){
                        for(uint32_t i = 0; i < ids.size(); i++){
                            auto shiftedInterval = shiftInterval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);
                            if(shiftedInterval.first > shiftedInterval.second){
                                ids.push_back(0);
                            } else {
                                ids.push_back(shiftedInterval.first);
                            }                            
                        }
                    } else {
                        for(uint32_t i = 0; i < ids.size(); i++){
                            if(ids[i] == 0){
                                continue;
                            }
                            auto shiftedInterval = shiftInterval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);
                            if(shiftedInterval.first > shiftedInterval.second){
                                ids[i] = 0;
                            } else {
                                ids[i] = std::max(ids[i], shiftedInterval.first);
                            }                            
                        }
                    }
                }
                return ids;
            }

            std::vector<uint32_t> getUpperIds() const {
                std::vector<uint32_t> ids;
                for(const auto& interval : _intervals){
                    if(ids.empty()){
                        ids = interval.getUpperIds();
                    } else {
                        for(uint32_t i = 0; i < ids.size(); i++){
                            ids[i] = std::max(ids[i], interval[i]._upper);
                        }
                    }
                }
                return ids;
            }

            std::vector<uint32_t> getUpperIds(int32_t modifier, const std::vector<size_t>& sizes) const {
                std::vector<uint32_t> ids;
                for(uint32_t j = 0; j < size(); j++){
                    const auto& interval = _intervals[j];
                    if(ids.empty()){
                        for(uint32_t i = 0; i < ids.size(); i++){
                            auto shiftedInterval = shiftInterval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);

                            if(shiftedInterval.first > shiftedInterval.second){
                                ids.push_back(sizes[i]-1);
                            } else {
                                ids.push_back(shiftedInterval.second);
                            }                            
                        }
                    } else {
                        for(uint32_t i = 0; i < ids.size(); i++){
                            if(ids[i] == sizes[i]-1){
                                continue;
                            }
                            auto shiftedInterval = shiftInterval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);

                            if(shiftedInterval.first > shiftedInterval.second){
                                ids[i] = sizes[i]-1;
                            } else {
                                ids[i] = std::max(ids[i], shiftedInterval.second);
                            }                            
                        }
                    }
                }
                return ids;
            }

            void applyModifier(int32_t modifier, const std::vector<size_t>& sizes) {
                std::vector<interval_t> collectedIntervals;
                for(auto& interval : _intervals){
                    std::vector<interval_t> newIntervals;
                    newIntervals.push_back(std::move(interval));
                    for(uint32_t i = 0; i < interval.size(); i++){
                        std::vector<interval_t> tempIntervals;
                        for(auto& interval1 : newIntervals){
                            auto shiftedInterval = shiftInterval(interval1[i]._lower, interval1[i]._upper, sizes[i], modifier);

                            if(shiftedInterval.first > shiftedInterval.second) {
                                auto newInterval = interval1;

                                interval1[i]._lower = 0;
                                interval1[i]._upper = shiftedInterval.second;

                                newInterval[i]._lower = shiftedInterval.first;
                                newInterval[i]._upper = sizes[i]-1;
                                tempIntervals.push_back(std::move(newInterval));
                            } else {
                                interval1[i]._lower = shiftedInterval.first;
                                interval1[i]._upper = shiftedInterval.second;
                            }
                        }
                        newIntervals.insert(newIntervals.end(), tempIntervals.begin(), tempIntervals.end());                                                
                    }
                    collectedIntervals.insert(collectedIntervals.end(), newIntervals.begin(), newIntervals.end());
                }
                
                _intervals = std::move(collectedIntervals);
            }

            bool contains(const interval_t &interval) const {
                for(auto &localInterval : _intervals){
                    if(localInterval.contains(interval)){
                        return true;
                    }
                }
                return false;
            }

            bool contains(const interval_t &interval, const std::vector<bool> &diagonalPositions) const {
                for(const auto &localInterval : _intervals){
                    if(localInterval.contains(interval, diagonalPositions)){
                        return true;
                    }
                }
                return false;
            }

            void removeInterval(const interval_t& interval) {
                for (uint32_t i = 0; i < _intervals.size(); i++) {
                    if(interval.equals(_intervals[i])){
                        removeInterval(i);
                        return;
                    }
                }
            }

            void removeInterval(uint32_t index) {               
                _intervals.erase(_intervals.begin() + index); 
            }

            

            void restrict(uint32_t k) {              
                simplify();
                if(k == 0){
                    return;
                }
                
                while (size() > k){ 
                    closestIntervals closestInterval = getClosestIntervals();
                    auto& interval = _intervals[closestInterval.intervalId1]; 
                    const auto& otherInterval = _intervals[closestInterval.intervalId2]; 

                    for(uint32_t l = 0; l < interval.size(); l++) {
                        interval[l] |= otherInterval[l];
                    }
                    
                    _intervals.erase(_intervals.begin() + closestInterval.intervalId2);
                }
                simplify();
            }

            closestIntervals getClosestIntervals() const {
                closestIntervals currentBest = {0,0, std::numeric_limits<uint32_t>::max()};
                    for (uint32_t i = 0; i < size()-1; i++) {
                        const auto& interval = _intervals[i];
                        for(uint32_t j = i+1; j < size(); j++){
                            const auto& otherInterval = _intervals[j];
                            uint32_t dist = 0;
                            
                            for(uint32_t k = 0; k < interval.size(); k++) {
                                int32_t val1 = otherInterval[k]._lower - interval[k]._upper;
                                int32_t val2 = interval[k]._lower - otherInterval[k]._upper;
                                dist += std::max(0, std::max(val1, val2));
                                if(dist >= currentBest.distance){
                                    break;
                                }
                            }
                            
                            if(dist < currentBest.distance){
                                currentBest.distance = dist;
                                currentBest.intervalId1 = i;
                                currentBest.intervalId2 = j;

                                //if the distance is 1 we cannot find any intervals that are closer so we stop searching
                                if(currentBest.distance == 1){
                                    return currentBest;
                                }
                            }                           
                        }
                    }
                return currentBest;
            }

            void simplify() {
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
                        const auto& otherInterval = _intervals[j];

                        if(!otherInterval.isSound()){
                            continue;
                        }   
                        bool overlap = true;

                        if(overlap){
                            for(uint32_t k = 0; k < interval.size(); k++) {                            
                                if(interval[k]._lower > otherInterval[k]._upper  || otherInterval[k]._lower > interval[k]._upper) {
                                    overlap = false;
                                    break;
                                }
                            }
                        }
                        
                        if(overlap) {
                            for(uint32_t l = 0; l < interval.size(); l++) {
                                interval[l] |= otherInterval[l];
                            }
                            rangesToRemove.insert(j);
                        }  
                    }
                }
                
                // well, ok. the right way to do this operation would be to do the
                // removal inline with the previous loop.
                // i.e. the previous loop would have to iterate backwards also.
                for (auto i = rangesToRemove.rbegin(); i != rangesToRemove.rend(); ++i) {
                    _intervals.erase(_intervals.begin() + *i);
                }
            }

            void combineNeighbours() {
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
                        const auto& otherInterval = _intervals[j];

                        if(!otherInterval.isSound()){
                            continue;
                        }   
                        bool overlap = true;

                        uint32_t dist = 1;

                        if(overlap){
                            for(uint32_t k = 0; k < interval.size(); k++) {                            
                                if(interval[k]._lower > otherInterval[k]._upper  || otherInterval[k]._lower > interval[k]._upper) {
                                    if(interval[k]._lower > otherInterval[k]._upper + dist  || 
                                        otherInterval[k]._lower > interval[k]._upper + dist) {
                                        overlap = false;
                                        break;
                                    } else {
                                        dist = 0;
                                    }                                    
                                }
                            }
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
            } 
        };
    }
}


#endif /* INTERVALS_H */