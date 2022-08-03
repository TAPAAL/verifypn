
#include "PetriEngine/Colored/EquivalenceVec.h"

namespace PetriEngine {
    namespace Colored {
        void EquivalenceVec::applyPartition(Colored::ArcIntervals& arcInterval) const{
            if(_diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back().type()->size(_diagonalTuplePositions)){
                return;
            }
            std::vector<Colored::interval_vector_t> newTupleVec;
            for(auto& intervalTuple : arcInterval._intervalTupleVec){
                intervalTuple.combineNeighbours();
                interval_vector_t newIntervalTuple;
                for(const auto& interval : intervalTuple){
                    for(const auto& EQClass : _equivalenceClasses){
                        for(const auto& EQinterval : EQClass.intervals()){
                            auto overlap = interval.getOverlap(EQinterval, _diagonalTuplePositions);
                            if(overlap.isSound()){
                                auto singleInterval = EQinterval.getCanonicalInterval();
                                for(uint32_t i = 0; i < _diagonalTuplePositions.size(); i++){
                                    if(_diagonalTuplePositions[i]){
                                        singleInterval[i] = interval[i];
                                    }
                                }
                                newIntervalTuple.addInterval(std::move(singleInterval));
                                break;
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
                    for(const auto &interval : _equivalenceClasses[i].intervals()){
                        if(!_equivalenceClasses[j].intervals().contains(interval, _diagonalTuplePositions)){
                            fullyContained = false;
                            break;
                        }
                    }
                    if(fullyContained){
                        _equivalenceClasses.erase(_equivalenceClasses.begin() + i);
                        break;
                    }
                }
            }
        }

        void EquivalenceVec::addColorToEqClassMap(const Color *color){
            for(auto& eqClass : _equivalenceClasses){
                std::vector<uint32_t> colorIds;
                color->getTupleId(colorIds);
                if(eqClass.containsColor(colorIds, _diagonalTuplePositions)){
                    _colorEQClassMap[color] = &eqClass;
                    break;
                }
            }
        }

        void EquivalenceVec::applyPartition(std::vector<uint32_t> &colorIds) const{
            if(_diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back().type()->size(_diagonalTuplePositions)){
                return;
            }

            interval_t interval;
            for(auto colorId : colorIds){
                interval.addRange(colorId, colorId);
            }

            for(const auto& EqClass : _equivalenceClasses){
                for(const auto& EqInterval : EqClass.intervals()){
                    if(EqInterval.contains(interval, _diagonalTuplePositions)){
                        auto singleInterval = EqInterval.getCanonicalInterval();
                        for(uint32_t i = 0; i < singleInterval.size(); i++){
                            colorIds[i] = _diagonalTuplePositions[i]? interval[i]._lower: singleInterval[i]._lower;
                        }
                    }
                }
            }
        }

        //Add color ids of diagonal positions as we represent partitions with diagonal postitions
        //as a single equivalence class to save space, but they should not be partition together
        const uint32_t EquivalenceVec::getUniqueIdForColor(const Colored::Color *color) const {
            PetriEngine::Colored::EquivalenceClass *eqClass = _colorEQClassMap.find(color)->second;

            std::vector<uint32_t> colorTupleIds;
            std::vector<uint32_t> newColorTupleIds;
            bool hasDiagonalPositions;
            color->getTupleId(colorTupleIds);
            for(uint32_t i = 0; i < colorTupleIds.size(); i++){
                if(_diagonalTuplePositions[i]){
                    newColorTupleIds.push_back(colorTupleIds[i]);
                } else {
                    hasDiagonalPositions = true;
                    newColorTupleIds.push_back(eqClass->intervals().back().getLowerIds()[i]);
                }
            }

            if(hasDiagonalPositions){
                return eqClass->id() + color->getColorType()->getColor(newColorTupleIds)->getId();
            }

            return eqClass->id();

        }
    }
}
