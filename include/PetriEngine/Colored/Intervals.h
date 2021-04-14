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


namespace PetriEngine {
    namespace Colored {

        struct interval_t {
            std::vector<Reachability::range_t> _ranges;

            interval_t() {
            }

            ~interval_t(){}

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

            void addRange(Reachability::range_t newRange) {
                _ranges.push_back(newRange);
            }

            void addRange(Reachability::range_t newRange, uint32_t index){
                _ranges.insert(_ranges.begin() + index, newRange);
            }

            void addRange(uint32_t l, uint32_t u) {
                _ranges.push_back(Reachability::range_t(l, u));
            }

            void addRange(uint32_t lower, uint32_t upper, uint32_t index){
                _ranges.insert(_ranges.begin() + index, Reachability::range_t(lower, upper));
            }

            Reachability::range_t& operator[] (size_t index) {
                return _ranges[index];
            }
            
            Reachability::range_t& operator[] (int index) {
                return _ranges[index];
            }
            
            Reachability::range_t& operator[] (uint32_t index) {
                assert(index < _ranges.size());
                return _ranges[index];
            }

            Reachability::range_t& getFirst() {
                return _ranges[0];
            }

            Reachability::range_t& getLast() {
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

            uint32_t getContainedColors(){
                uint32_t colors = 1;
                for(auto range: _ranges) {
                    colors *= 1+  range._upper - range._lower;
                }          
                return colors;
            }

            bool contains(interval_t other){
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

            interval_t getOverlap(interval_t other){
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

            void print() {
                for(auto range : _ranges){
                    std::cout << " " << range._lower << "-" << range._upper << " ";
                }
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

            intervalTuple_t(std::vector<interval_t> ranges) :  _intervals(ranges) {
            };

            interval_t& getFirst(){
                return _intervals[0];
            }

            interval_t& back() {
                return _intervals.back();
            }

            size_t size() {
                return _intervals.size();
            }

            size_t tupleSize() {
                return _intervals[0].size();
            }

            uint32_t getContainedColors(){
                uint32_t colors = 0;
                for (auto interval : _intervals) {
                    colors += interval.getContainedColors();              
                }
                return colors;
            }

            std::pair<uint32_t,uint32_t> shiftInterval(uint32_t lower, uint32_t upper, uint32_t ctSize, int32_t modifier){
                int32_t lower_val = ctSize + (lower + modifier);
                int32_t upper_val = ctSize + (upper + modifier);
                return std::make_pair(lower_val % ctSize, upper_val % ctSize);
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
                            break;
                        }
                    }
                    if(rangeEnd) {
                        if(j+1 != _intervals.size()) {
                            return _intervals[j+1];
                        } else {
                            return getFirst();
                        }
                    }
                }
                return interval_t();
            }

            std::vector<Colored::interval_t> shrinkIntervals(uint32_t newSize){
                std::vector<Colored::interval_t> resizedIntervals;
                for(auto interval : _intervals){
                    Colored::interval_t resizedInterval;
                    for(uint32_t i = 0; i < newSize; i++){
                        resizedInterval.addRange(interval[i]);
                    }
                    resizedIntervals.push_back(resizedInterval);
                }
                return resizedIntervals;
            }

            void addInterval(interval_t interval) {
                uint32_t vecIndex = 0;

                if(!_intervals.empty()) {
                    assert(_intervals[0].size() == interval.size());
                } else {
                    _intervals.push_back(interval);
                    return;
                }

                for (auto& localInterval : _intervals) {
                    bool overlap = true;
                    enum FoundPlace {undecided, greater, lower};
                    FoundPlace foundPlace = undecided;

                    for(uint32_t k = 0; k < interval.size(); k++){
                        if(interval[k]._lower > localInterval[k]._upper  || localInterval[k]._lower > interval[k]._upper){
                            overlap = false;
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
                        if(!overlap && foundPlace != undecided){
                            break;
                        }
                    }

                    if(overlap) {
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

            void constrainLower(std::vector<uint32_t> values, bool strict) {
                for(uint32_t i = 0; i < _intervals.size(); i++) {
                    for(uint32_t j = 0; j < values.size(); j++){
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

            void constrainUpper(std::vector<uint32_t> values, bool strict) {
                for(uint32_t i = 0; i < _intervals.size(); i++) {
                    for(uint32_t j = 0; j < values.size(); j++){
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
                std::vector<uint32_t> ids;
                for(auto interval : _intervals){
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

            std::vector<uint32_t> getLowerIds(int32_t modifier, std::vector<size_t> sizes){
                std::vector<uint32_t> ids;
                for(uint32_t j = 0; j < size(); j++){
                    auto interval = &_intervals[j];
                    if(ids.empty()){
                        for(uint32_t i = 0; i < ids.size(); i++){
                            auto shiftedInterval = shiftInterval(interval->operator[](i)._lower, interval->operator[](i)._upper, sizes[i], modifier);
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
                            auto shiftedInterval = shiftInterval(interval->operator[](i)._lower, interval->operator[](i)._upper, sizes[i], modifier);
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

            std::vector<uint32_t> getUpperIds(){
                std::vector<uint32_t> ids;
                for(auto interval : _intervals){
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

            std::vector<uint32_t> getUpperIds(int32_t modifier, std::vector<size_t> sizes){
                std::vector<uint32_t> ids;
                for(uint32_t j = 0; j < size(); j++){
                    auto interval = &_intervals[j];
                    if(ids.empty()){
                        for(uint32_t i = 0; i < ids.size(); i++){
                            auto shiftedInterval = shiftInterval(interval->operator[](i)._lower, interval->operator[](i)._upper, sizes[i], modifier);

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
                            auto shiftedInterval = shiftInterval(interval->operator[](i)._lower, interval->operator[](i)._upper, sizes[i], modifier);

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

            void applyModifier(int32_t modifier, std::vector<size_t> sizes){
                std::vector<interval_t> collectedIntervals;
                for(auto& interval : _intervals){
                    std::vector<interval_t> newIntervals;
                    newIntervals.push_back(std::move(interval));
                    for(uint32_t i = 0; i < interval.size(); i++){
                        std::vector<interval_t> tempIntervals;
                        for(auto& interval1 : newIntervals){
                            auto shiftedInterval = shiftInterval(interval1[i]._lower, interval1[i]._upper, sizes[i], modifier);

                            if(shiftedInterval.first > shiftedInterval.second){
                                auto newInterval = interval1;

                                interval1[i]._lower = 0;
                                interval1[i]._upper = shiftedInterval.second;

                                newInterval[i]._lower = shiftedInterval.first;
                                newInterval[i]._upper = sizes[i]-1;
                                tempIntervals.push_back(std::move(newInterval));
                            }else {
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

            bool contains(interval_t interval){
                for(auto localInterval : _intervals){
                    if(localInterval.contains(interval)){
                        return true;
                    }
                }
                return false;
            }

            void removeInterval(interval_t interval) {
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

            

            void restrict(uint32_t k){              
                simplify();
                if(k == 0){
                    return;
                }
                
                while (size() > k){ 
                    closestIntervals closestInterval = getClosestIntervals();
                    auto interval = &_intervals[closestInterval.intervalId1]; 
                    auto otherInterval = &_intervals[closestInterval.intervalId2]; 

                    for(uint32_t l = 0; l < interval->size(); l++) {
                        interval->operator[](l) |= otherInterval->operator[](l);
                    }
                    
                    _intervals.erase(_intervals.begin() + closestInterval.intervalId2);
                }
                simplify();
            }

            closestIntervals getClosestIntervals(){
                closestIntervals currentBest = {0,0, UINT32_MAX};
                    for (uint32_t i = 0; i < size()-1; i++) {
                        auto interval = &_intervals[i];
                        for(uint32_t j = i+1; j < size(); j++){
                            auto otherInterval = &_intervals[j];
                            uint32_t dist = 0;
                            
                            for(uint32_t k = 0; k < interval->size(); k++) {
                                int32_t val1 = otherInterval->operator[](k)._lower - interval->operator[](k)._upper;
                                int32_t val2 = interval->operator[](k)._lower - otherInterval->operator[](k)._upper;
                                // int32_t lowerDist = interval->operator[](k)._lower > otherInterval->operator[](k)._lower? 
                                //     interval->operator[](k)._lower - otherInterval->operator[](k)._lower : 
                                //     otherInterval->operator[](k)._lower - interval->operator[](k)._lower;
                                // int32_t upperDist = interval->operator[](k)._upper > otherInterval->operator[](k)._upper? 
                                //     interval->operator[](k)._upper - otherInterval->operator[](k)._upper : 
                                //     otherInterval->operator[](k)._upper - interval->operator[](k)._upper;
                                dist += std::max(0, std::max(val1, val2));
                                //dist += std::max(upperDist + lowerDist, upperDist + lowerDist + std::max(val1, val2));
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
                    auto interval = &_intervals[i];
                    if(!interval->isSound()){
                        rangesToRemove.insert(i);
                        continue;
                    }   
                    for(uint32_t j = i+1; j < _intervals.size(); j++){
                        auto otherInterval = &_intervals[j];

                        if(!otherInterval->isSound()){
                            continue;
                        }   
                        bool overlap = true;

                        if(overlap){
                            for(uint32_t k = 0; k < interval->size(); k++) {                            
                                if(interval->operator[](k)._lower > otherInterval->operator[](k)._upper  || otherInterval->operator[](k)._lower > interval->operator[](k)._upper) {
                                    overlap = false;
                                    break;
                                }
                            }
                        }
                        
                        if(overlap) {
                            for(uint32_t l = 0; l < interval->size(); l++) {
                                interval->operator[](l) |= otherInterval->operator[](l);
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