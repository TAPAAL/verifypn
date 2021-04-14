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

            interval_t getSingleColorInterval(){
                interval_t newInterval;
                for(auto id : getLowerIds()){
                    newInterval.addRange(id,id);
                }
                return newInterval;
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

            std::vector<interval_t> removeInterval(interval_t interval){
                std::vector<interval_t> res;
                //The interval should have the same upper and lower ids
                auto ids = interval.getLowerIds();

                for(uint32_t i = 0; i < size(); i++){
                    if(_ranges[i]._lower < ids[i] && _ranges[i]._upper > ids[i]){
                        if(res.empty()){
                            interval_t newLowerInterval;
                            interval_t newUpperInterval;
                            newLowerInterval.addRange(_ranges[i]._lower, ids[i]-1);
                            newUpperInterval.addRange(ids[i]+1, _ranges[i]._upper);

                            res.push_back(newLowerInterval);
                            res.push_back(newUpperInterval);
                        } else {
                            std::vector<interval_t> newIntervals;
                            for(auto& interval : res){
                                interval_t intervalCopy = interval;
                                interval.addRange(_ranges[i]._lower, ids[i]-1);
                                intervalCopy.addRange(ids[i]+1, _ranges[i]._upper);
                                newIntervals.push_back(intervalCopy);
                            }
                            res.insert(res.end(), newIntervals.begin(), newIntervals.end());
                        }
                    } else if (_ranges[i]._lower < ids[i] && _ranges[i]._upper == ids[i]){
                        if(res.empty()){
                            interval_t newInterval;
                            newInterval.addRange(_ranges[i]._lower, ids[i]-1);

                            res.push_back(newInterval);
                        } else {
                            for(auto& interval : res){
                                interval.addRange(_ranges[i]._lower, ids[i]-1);
                            }
                        }
                    } else if (_ranges[i]._lower == ids[i] && _ranges[i]._upper > ids[i]) {
                        if(res.empty()){
                            interval_t newInterval;
                            newInterval.addRange(ids[i]+1, _ranges[i]._upper);

                            res.push_back(newInterval);
                        } else {
                            for(auto& interval : res){
                                interval.addRange(ids[i]+1, _ranges[i]._upper);
                            }
                        }
                    } else {
                        if(res.empty()){
                            interval_t newInterval;
                            newInterval.addRange(_ranges[i]);

                            res.push_back(newInterval);
                        } else {
                            for(auto& interval : res){
                                interval.addRange(_ranges[i]);
                            }
                        }
                    }
                }

                return res;
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

            std::vector<interval_t> getSubtracted(interval_t other, uint32_t ctSize){
                std::vector<interval_t> result;
                
                if(size() != other.size()){
                    return result;
                }
                
                for(uint32_t i = 0; i < size(); i++){
                    interval_t newInterval = *this;
                    
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

            void print() {
                for(auto range : _ranges){
                    std::cout << " " << range._lower << "-" << range._upper << " ";
                }
            }

            std::string toString() {
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

            void inEquality(intervalTuple_t &other){

                if((size() > 1 || getFirst().intervalCombinations() > 1) && (other.size() > 1 || other.getFirst().intervalCombinations() > 1)){
                    return;
                } else if(size() > 1 || getFirst().intervalCombinations() > 1 ) {
                    std::vector<interval_t> newIntervals;
                    for(auto& interval : _intervals){
                        auto remainingIntervals = interval.removeInterval(other.getFirst());
                        
                        newIntervals.insert(newIntervals.end(), remainingIntervals.begin(), remainingIntervals.end());
                    }
                } else if(other.size() > 1 || other.getFirst().intervalCombinations() > 1 ) {
                    std::vector<interval_t> newIntervals;
                    for(auto& interval : other._intervals){
                        auto remainingIntervals = interval.removeInterval(getFirst());
                        
                        newIntervals.insert(newIntervals.end(), remainingIntervals.begin(), remainingIntervals.end());
                    }
                } else {
                    if(getLowerIds() == other.getLowerIds()){
                        other.getFirst().getFirst().invalidate();
                        getFirst().getFirst().invalidate();
                    }
                }
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
                    bool extendsInterval = true;
                    enum FoundPlace {undecided, greater, lower};
                    FoundPlace foundPlace = undecided;

                    //uint32_t allowedDist = 1;

                    for(uint32_t k = 0; k < interval.size(); k++){
                        if(interval[k]._lower > localInterval[k]._upper  || localInterval[k]._lower > interval[k]._upper){
                            //if(interval[k]._lower > localInterval[k]._upper + allowedDist  || localInterval[k]._lower > interval[k]._upper + allowedDist){
                                extendsInterval = false;
                            // } else {
                            //     allowedDist = 0;
                            // }
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

            std::string toString() {
                std::string out;
                for (auto interval : _intervals){
                    out += "[";
                    out += interval.toString();
                    out += "]\n";
                }
                return out;
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

            void combineNeighbours() {
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

                        uint32_t dist = 1;

                        if(overlap){
                            for(uint32_t k = 0; k < interval->size(); k++) {                            
                                if(interval->operator[](k)._lower > otherInterval->operator[](k)._upper  || otherInterval->operator[](k)._lower > interval->operator[](k)._upper) {
                                    if(interval->operator[](k)._lower > otherInterval->operator[](k)._upper + dist  || otherInterval->operator[](k)._lower > interval->operator[](k)._upper + dist) {
                                        overlap = false;
                                        break;
                                    } else {
                                        dist = 0;
                                    }                                    
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