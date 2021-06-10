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


        EquivalenceClass EquivalenceClass::subtract(const EquivalenceClass &other, const std::vector<bool> &diagonalPositions){
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
                    }
                }
                for(auto interval : intervalSubRes._intervals){
                    resIntervals.addInterval(interval);
                }                
            }
            result._colorIntervals = resIntervals;                  
            return result;
        }

        bool EquivalenceClass::containsColor(const std::vector<uint32_t> &ids, const std::vector<bool> &diagonalPositions){
            if(ids.size() != _colorIntervals.getFirstConst().size()){
                return false;
            }
            for(auto &interval : _colorIntervals._intervals){
                bool contained = true;
                for(uint32_t i = 0; i < ids.size(); i++){
                    if(!interval[i].contains(ids[i])){
                        contained = false;
                        break;
                    }
                }
                if(contained){
                    return true;
                }
            }
            return false;
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