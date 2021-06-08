#ifndef PARTITION_H
#define PARTITION_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceClass {
            public:
                EquivalenceClass();
                EquivalenceClass(ColorType *colorType);
                EquivalenceClass(ColorType *colorType, intervalTuple_t colorIntervals);
                ~EquivalenceClass() {}
                std::string toString(){
                    return _colorIntervals.toString();
                }

                bool isEmpty(){
                    if(_colorIntervals.size() < 1 || _colorIntervals.getFirst().size() < 1){
                        return true;
                    } 
                    return false;
                }

                bool containsColor(std::vector<uint32_t> ids, const std::vector<bool> &diagonalPositions);

                size_t size();

                EquivalenceClass intersect(EquivalenceClass other);

                EquivalenceClass subtract(EquivalenceClass other, const std::vector<bool> &diagonalPositions,  bool print);

                static uint32_t idCounter;
                uint32_t _id;
                ColorType *_colorType;
                intervalTuple_t _colorIntervals;

            private:

            
        };

        struct EquivalenceVec{
            std::vector<EquivalenceClass> _equivalenceClasses;
            std::unordered_map<const Colored::Color *, EquivalenceClass *> colorEQClassMap;
            std::vector<bool> diagonalTuplePositions;
            bool diagonal = false;

            void applyPartition(Colored::ArcIntervals& arcInterval){
                if(diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back()._colorType->size(&diagonalTuplePositions)){
                    diagonal = true;
                    return;
                }
                std::vector<Colored::intervalTuple_t> newTupleVec;
                for(auto intervalTuple : arcInterval._intervalTupleVec){
                    intervalTuple.combineNeighbours();
                    intervalTuple_t newIntervalTuple;
                    for(auto interval : intervalTuple._intervals){
                        for(auto EQClass : _equivalenceClasses){
                            for(auto EQinterval : EQClass._colorIntervals._intervals){
                                auto overlap = interval.getOverlap(EQinterval, diagonalTuplePositions);
                                if(overlap.isSound()){
                                    auto singleInterval = EQinterval.getSingleColorInterval(); 
                                    for(uint32_t i = 0; i < diagonalTuplePositions.size(); i++){
                                        if(diagonalTuplePositions[i]){
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

            void mergeEqClasses(){
                std::set<uint32_t> indexesForRemoval;
                for(uint32_t i = 0; i < _equivalenceClasses.size()-1; i++){
                    for(uint32_t j = i+1; j < _equivalenceClasses.size(); j++){
                        bool fullyContained = true;
                        for(auto interval : _equivalenceClasses[j]._colorIntervals._intervals){
                            if(!_equivalenceClasses[i]._colorIntervals.contains(interval, diagonalTuplePositions)){
                                fullyContained = false;
                                break;
                            }
                        }
                        if(fullyContained){
                            indexesForRemoval.insert(j);
                        }
                    } 
                }
                for(auto index = indexesForRemoval.rbegin(); index != indexesForRemoval.rend(); ++index){
                    _equivalenceClasses.erase(_equivalenceClasses.begin() + *index);
                }
            }

            void applyPartition(std::vector<uint32_t> *colorIds){
                if(diagonal || _equivalenceClasses.size() >= _equivalenceClasses.back()._colorType->size(&diagonalTuplePositions)){
                    diagonal = true;
                    return;
                }

                interval_t interval; 
                for(auto colorId : *colorIds){
                    interval.addRange(colorId, colorId);
                }

                for(auto EqClass : _equivalenceClasses){
                    for(auto EqInterval : EqClass._colorIntervals._intervals){
                        if(EqInterval.contains(interval, diagonalTuplePositions)){
                            auto singleInterval = EqInterval.getSingleColorInterval(); 
                            for(uint32_t i = 0; i < singleInterval.size(); i++){
                                colorIds->operator[](i) = diagonalTuplePositions[i]? interval[i]._lower: singleInterval[i]._lower;
                            }
                        }
                    }
                }
            }

            // void setDiagonal(uint32_t position, uint32_t numColors){
            //     std::vector<EquivalenceClass> newEqClasses;
            //     for(auto eqClass : _equivalenceClasses){
            //         for(uint i = 0; i < numColors; i++){
            //             auto newEqClass = EquivalenceClass(eqClass._colorType);
            //             for(auto interval : eqClass._colorIntervals._intervals){
            //                 if(interval[position].contains(i)){
            //                     interval_t newInterval = interval;
            //                     newInterval[position]._lower = i;
            //                     newInterval[position]._upper = i;
            //                     newEqClass._colorIntervals.addInterval(newInterval);
            //                 }                          
            //             }
            //             if(!newEqClass._colorIntervals._intervals.empty()){
            //                 newEqClasses.push_back(std::move(newEqClass));
            //             }
            //         }
            //     }
            //     _equivalenceClasses = std::move(newEqClasses);
            // }
        };

    }
}

#endif /* PARTITION_H */