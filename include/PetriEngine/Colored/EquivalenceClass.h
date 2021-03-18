#ifndef PARTITION_H
#define PARTITION_H

#include "Intervals.h"
#include "Colors.h"

namespace PetriEngine {
    namespace Colored {
        struct EquivalenceClass {
            ColorType *_colorType;
            intervalTuple_t _colorIntervals;
            
            std::string toString(){
                return _colorIntervals.toString();
            }

            EquivalenceClass intersect(EquivalenceClass other){
                EquivalenceClass result;
                if(_colorType != other._colorType){
                    return result;
                }
                result._colorType = _colorType;

                for(auto interval : _colorIntervals._intervals){
                    for(auto otherInterval : other._colorIntervals._intervals){
                        auto overlappingInterval = interval.getOverlap(otherInterval);
                        if(overlappingInterval.isSound()){
                            result._colorIntervals.addInterval(overlappingInterval);
                        }
                    }
                }
                return result;
            }

            EquivalenceClass subtract(EquivalenceClass other){
                EquivalenceClass result;
                if(_colorType != other._colorType){
                    return result;
                }
                result._colorType = _colorType;
                intervalTuple_t resIntervals;
                for(auto interval : _colorIntervals._intervals){
                    
                    for(auto otherInterval : other._colorIntervals._intervals){
                        auto subtractedIntervals = interval.getSubtracted(otherInterval, _colorType->size());
                        // std::cout << interval.toString() << " - " << otherInterval.toString() << " = " << std::endl;
                        // for(auto subinter : subtractedIntervals){
                        //     std::cout << subinter.toString() << std::endl;
                        // }
                        // std::cout << "~~~~~~" << std::endl;

                        if(subtractedIntervals.empty() || subtractedIntervals[0].size() == 0){
                            if(_colorType->productSize() > 1){
                                resIntervals._intervals.clear();
                                break;
                            } else {
                                continue;
                            }                            
                        }

                        if(resIntervals._intervals.empty()){
                            resIntervals._intervals = subtractedIntervals;
                        } else {
                            // intervalTuple_t newIntervals;
                            // for(auto newInterval : subtractedIntervals){
                            //     for(auto interval : resIntervals._intervals){
                            //         auto intersection = interval.getOverlap(newInterval);
                            //         if(intersection.isSound()){
                            //             newIntervals.addInterval(intersection);
                            //         }
                            //     }
                            // }
                            // resIntervals = std::move(newIntervals);   
                            for(auto newInterval : subtractedIntervals){
                                resIntervals.addInterval(newInterval);
                            }                            
                        }
                        //std::cout << "Subtract res is now: " << resIntervals.toString() << std::endl;
                    }
                }
                result._colorIntervals = resIntervals;                  
                return result;
            }

            bool isEmpty(){
                if(_colorIntervals.size() < 1 || _colorIntervals.getFirst().size() < 1){
                    return true;
                } 
                return false;
            }

            bool constainsColor(std::vector<uint32_t> ids){
                interval_t interval;
                for(auto id : ids){
                    interval.addRange(id,id);
                }
                return _colorIntervals.contains(interval);
            }

            size_t size(){
                size_t result = 0;
                for(auto interval : _colorIntervals._intervals){
                    size_t intervalSize = 1;
                    for(auto range : interval._ranges){
                        intervalSize *= range.size();
                    }
                    result += intervalSize;
                }
                return result;
            }

        };

        struct EquivalenceVec{
            std::vector<EquivalenceClass> _equivalenceClasses;
            bool diagonal = false;
        };

    }
}

#endif /* PARTITION_H */