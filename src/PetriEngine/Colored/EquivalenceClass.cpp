#include "PetriEngine/Colored/EquivalenceClass.h"

namespace PetriEngine {
    namespace Colored {

        uint32_t EquivalenceClass::idCounter = 0;

        EquivalenceClass::EquivalenceClass() : _id(++idCounter){
        }
        EquivalenceClass::EquivalenceClass(ColorType *colorType) 
        : _id(++idCounter), _colorType(colorType){
        }
        EquivalenceClass::EquivalenceClass(ColorType *colorType, intervalTuple_t colorIntervals) 
        : _id(++idCounter), _colorType(colorType), _colorIntervals(colorIntervals){
        }

        EquivalenceClass EquivalenceClass::intersect(EquivalenceClass other){
            EquivalenceClass result = EquivalenceClass();

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


        EquivalenceClass EquivalenceClass::subtract(EquivalenceClass other, const std::vector<bool> &diagonalPositions, bool print){
            EquivalenceClass result = EquivalenceClass();
            if(_colorType != other._colorType){
                return result;
            }
            result._colorType = _colorType;
            intervalTuple_t resIntervals;
            for(auto interval : _colorIntervals._intervals){ 
                intervalTuple_t intervalSubRes;                   
                for(auto otherInterval : other._colorIntervals._intervals){
                    auto subtractedIntervals = interval.getSubtracted(otherInterval, diagonalPositions, _colorType->size());
                    if(print){
                        std::cout << interval.toString() << " - " << otherInterval.toString() << " = " << std::endl;
                        for(auto subinter : subtractedIntervals){
                            std::cout << subinter.toString() << std::endl;
                        }
                        std::cout << "~~~~~~" << std::endl;
                    }
                    

                    if(subtractedIntervals.empty() || subtractedIntervals[0].size() == 0){
                        intervalSubRes._intervals.clear();
                        break;                           
                    }

                    if(intervalSubRes._intervals.empty()){
                        intervalSubRes._intervals = subtractedIntervals;
                    } else {
                        intervalTuple_t newIntervals;
                        for(auto newInterval : subtractedIntervals){
                            for(auto interval : intervalSubRes._intervals){
                                auto intersection = interval.getOverlap(newInterval);
                                if(intersection.isSound()){
                                    newIntervals.addInterval(intersection);
                                }
                            }
                        }
                        intervalSubRes = std::move(newIntervals);   
                        // for(auto newInterval : subtractedIntervals){
                        //     resIntervals.addInterval(newInterval);
                        // }                            
                    }
                    if(print){
                        std::cout << "intermediate res is now: " << intervalSubRes.toString() << std::endl;
                    }
                }
                if(print) std::cout << "Done looping inner" << std::endl;
                for(auto interval : intervalSubRes._intervals){
                    resIntervals.addInterval(interval);
                }
                if(print){
                    std::cout << "Subtract res is now: " << resIntervals.toString() << std::endl;
                }
                
            }
            result._colorIntervals = resIntervals;                  
            return result;
        }

        bool EquivalenceClass::containsColor(std::vector<uint32_t> ids, const std::vector<bool> &diagonalPositions){
            interval_t interval;
            for(auto id : ids){
                interval.addRange(id,id);
            }
            return _colorIntervals.contains(interval, diagonalPositions);
        }

        size_t EquivalenceClass::size(){
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
    }
}