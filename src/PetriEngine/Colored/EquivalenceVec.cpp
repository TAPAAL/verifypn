
#include "PetriEngine/Colored/EquivalenceVec.h"

namespace PetriEngine {
    namespace Colored {
        void EquivalenceVec::applyPartition(Colored::ArcIntervals& arcInterval) const{
            if(_diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back()._colorType->size(_diagonalTuplePositions)){
                return;
            }
            std::vector<Colored::intervalTuple_t> newTupleVec;
            for(auto intervalTuple : arcInterval._intervalTupleVec){
                intervalTuple.combineNeighbours();
                intervalTuple_t newIntervalTuple;
                for(auto interval : intervalTuple._intervals){
                    for(auto EQClass : _equivalenceClasses){
                        for(auto EQinterval : EQClass._colorIntervals._intervals){
                            auto overlap = interval.getOverlap(EQinterval, _diagonalTuplePositions);
                            if(overlap.isSound()){
                                auto singleInterval = EQinterval.getSingleColorInterval(); 
                                for(uint32_t i = 0; i < _diagonalTuplePositions.size(); i++){
                                    if(_diagonalTuplePositions[i]){
                                        singleInterval[i] = interval[i];
                                    }
                                }
                                newIntervalTuple.addInterval(std::move(singleInterval));
                                continue;
                            }
                        }
                    }
                }
                newTupleVec.push_back(std::move(newIntervalTuple));
            }
            arcInterval._intervalTupleVec = std::move(newTupleVec);               
        }

        void EquivalenceVec::mergeEqClasses(){
            for(int32_t i = _equivalenceClasses.size()-1; i >= 1; i--){
                for(int32_t j = i-1; j >= 0; j--){
                    bool fullyContained = true;
                    for(const auto &interval : _equivalenceClasses[i]._colorIntervals._intervals){
                        if(!_equivalenceClasses[j]._colorIntervals.contains(interval, _diagonalTuplePositions)){
                            fullyContained = false;
                            break;
                        }
                    }
                    if(fullyContained){
                        _equivalenceClasses.erase(_equivalenceClasses.begin() + i);
                    }
                } 
            }
        }

        void EquivalenceVec::addColorToEqClassMap(const Color *color){
            for(auto& eqClass : _equivalenceClasses){   
                std::vector<uint32_t> colorIds;
                color->getTupleId(&colorIds);
                if(eqClass.containsColor(colorIds, _diagonalTuplePositions)){
                    _colorEQClassMap[color] = &eqClass;
                    break;
                }
            } 
        }

        void EquivalenceVec::applyPartition(std::vector<uint32_t> &colorIds) const{
            if(_diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back()._colorType->size(_diagonalTuplePositions)){
                return;
            }

            interval_t interval; 
            for(auto colorId : colorIds){
                interval.addRange(colorId, colorId);
            }

            for(auto EqClass : _equivalenceClasses){
                for(auto EqInterval : EqClass._colorIntervals._intervals){
                    if(EqInterval.contains(interval, _diagonalTuplePositions)){
                        auto singleInterval = EqInterval.getSingleColorInterval(); 
                        for(uint32_t i = 0; i < singleInterval.size(); i++){
                            colorIds[i] = _diagonalTuplePositions[i]? interval[i]._lower: singleInterval[i]._lower;
                        }
                    }
                }
            }
        }
    }
}