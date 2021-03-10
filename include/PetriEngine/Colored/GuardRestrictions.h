#ifndef GuardRestrictions_H
#define GuardRestrictions_H

#include "Colors.h"
#include "Multiset.h"
#include <unordered_map>
#include <set>
#include <stdlib.h>

namespace PetriEngine {
    namespace Colored {

        struct guardRestrictor {

            int32_t getVarModifier(std::unordered_map<uint32_t, int32_t> *modPairMap, uint32_t index){
                int32_t modifier;
                for(auto idModPair : *modPairMap){
                    if(idModPair.first == index){
                        modifier = idModPair.second;
                        break;
                    }
                }
                return modifier;
            }

            Colored::interval_t getIntervalFromIds(std::vector<uint32_t> *idVec, uint32_t ctSize, int32_t modifier){
                Colored::interval_t interval;
                for(auto id : *idVec){          
                    int32_t val = ctSize + (id + modifier);
                    auto colorVal = val % ctSize;                  
                    interval.addRange(colorVal,colorVal);
                }
                return interval; 
            }

            Colored::intervalTuple_t getIntervalOverlap(std::vector<Colored::interval_t> *intervals1, std::vector<Colored::interval_t> *intervals2){
                Colored::intervalTuple_t newIntervalTuple;
                for(auto mainInterval : *intervals1){
                    for(auto otherInterval : *intervals2){
                        auto intervalOverlap = otherInterval.getOverlap(mainInterval);

                        if(intervalOverlap.isSound()){
                            newIntervalTuple.addInterval(intervalOverlap);
                        }
                    }
                }
                return newIntervalTuple;
            }

            void expandIdVec(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t> *varMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *otherVar, 
                            std::vector<uint32_t> &idVec, size_t targetSize, uint32_t index){
                while(idVec.size() < targetSize){
                    if(varPositions->count(index)){
                        auto rightTupleInterval = &varMap->operator[](varPositions->operator[](index));
                        int32_t rightVarMod = getVarModifier(&mainVarModifierMap->operator[](varPositions->operator[](index)).back(), index);
                        auto ids = rightTupleInterval->getUpperIds(-rightVarMod, varPositions->operator[](index)->colorType->getConstituentsSizes());
                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                        index += varPositions->operator[](index)->colorType->productSize();
                    } else {
                        auto oldSize = idVec.size();
                        constantMap->operator[](index)->getTupleId(&idVec); 
                        int32_t leftVarMod = getVarModifier(&otherVarModifierMap->operator[](otherVar).back(), index);

                        for(auto& id : idVec){
                            id = (otherVar->colorType->size()+(id + leftVarMod)) % otherVar->colorType->size();
                        }                                   
                        index+= idVec.size() - oldSize;
                    }
                }
            }

            void expandIntervalVec(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t> varMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *otherVar, 
                            std::vector<Colored::interval_t> &intervalVec, size_t targetSize, uint32_t index){

                while(intervalVec.size() < targetSize){
                    if(varPositions->count(index)){
                        auto rightTupleInterval = varMap[varPositions->operator[](index)];
                        int32_t rightVarMod = getVarModifier(&mainVarModifierMap->operator[](varPositions->operator[](index)).back(), index);
                        rightTupleInterval.applyModifier(-rightVarMod, varPositions->operator[](index)->colorType->getConstituentsSizes());
                        intervalVec.insert(intervalVec.end(), rightTupleInterval._intervals.begin(), rightTupleInterval._intervals.end());
                        index += varPositions->operator[](index)->colorType->productSize();
                    } else {
                        std::vector<uint32_t> colorIdVec;
                        constantMap->operator[](index)->getTupleId(&colorIdVec);
                        int32_t leftVarModifier = getVarModifier(&otherVarModifierMap->operator[](otherVar).back(), index);

                        for(auto id : colorIdVec){
                            for(auto& interval : intervalVec){
                                int32_t val = otherVar->colorType->size() + (id + leftVarModifier);
                                auto colorVal = val % otherVar->colorType->size(); 
                                interval.addRange(colorVal,colorVal);
                            }
                        }                                    
                        index+= colorIdVec.size();
                    }
                }
            }

            void restrictByConstant(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *var,
                            const Colored::Variable *otherVar, 
                            uint32_t index, bool lessthan, bool strict){
                for(auto& varMap : variableMap){
                    auto leftColor = constantMap->operator[](index);
                    auto rightTupleInterval = &varMap[var];
                    std::vector<uint32_t> idVec;
                    leftColor->getTupleId(&idVec);

                    expandIdVec(&varMap, mainVarModifierMap, otherVarModifierMap, varPositions, constantMap, otherVar, idVec, rightTupleInterval->tupleSize(), index + idVec.size());

                    if(lessthan){
                        rightTupleInterval->constrainUpper(idVec, strict);
                    } else {
                        rightTupleInterval->constrainLower(idVec, strict);
                    }
                }                 
            }

            void restrictEqByConstant(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositions,
                            std::unordered_map<uint32_t, const Color*> *constantMap,
                            const Colored::Variable *var,
                            uint32_t index){
                for(auto& varMap : variableMap){
                    auto color = constantMap->operator[](index);
                    auto tupleInterval = &varMap[var];
                    std::vector<uint32_t> idVec;
                    color->getTupleId(&idVec);
                    int32_t varModifier = getVarModifier(&otherVarModifierMap->operator[](var).back(), index);

                    std::vector<Colored::interval_t> intervals;
                    intervals.push_back(getIntervalFromIds(&idVec, var->colorType->size(), varModifier));

                    expandIntervalVec(varMap, mainVarModifierMap, otherVarModifierMap, varPositions, constantMap, var, intervals, tupleInterval->tupleSize(), index + idVec.size());

                    Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(&tupleInterval->_intervals, &intervals);
                    *tupleInterval = newIntervalTupleL;
                }
                
            }

            void restrictDiagonal(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index, bool lessthan, bool strict){

                diagonalVars.insert(var);
                diagonalVars.insert(varPositionsR->operator[](index));

                for(auto& varMap : variableMap){
                    if(varMap.count(var) == 0){
                        std::cout << "Unable to find left var " << var->name << std::endl;
                    }
                    if(varMap.count(varPositionsR->operator[](index)) == 0){
                        std::cout << "Unable to find right var " << varPositionsR->operator[](index)->name << std::endl;
                    }

                    auto leftTupleInterval = &varMap[var];
                    auto rightTupleInterval = &varMap[varPositionsR->operator[](index)];
                    int32_t leftVarModifier = getVarModifier(&varModifierMapL->operator[](var).back(), index);
                    int32_t rightVarModifier = getVarModifier(&varModifierMapR->operator[](varPositionsR->operator[](index)).back(), index);

                    auto leftIds = leftTupleInterval->getLowerIds(-leftVarModifier, var->colorType->getConstituentsSizes());
                    auto rightIds = rightTupleInterval->getUpperIds(-rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                    //comparing vars of same size
                    if(var->colorType->productSize() == varPositionsR->operator[](index)->colorType->productSize()){
                        if(lessthan){
                            leftTupleInterval->constrainUpper(rightIds, strict);
                            rightTupleInterval->constrainLower(leftIds, strict);
                        } else {
                            leftTupleInterval->constrainLower(rightIds, strict);
                            rightTupleInterval->constrainUpper(leftIds, strict);
                        }
                    } else if(var->colorType->productSize() > varPositionsR->operator[](index)->colorType->productSize()){
                        std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                        
                        auto idVec = rightIds;

                        expandIdVec(&varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, var, idVec, leftTupleInterval->tupleSize(), index + varPositionsR->operator[](index)->colorType->productSize());

                        if(lessthan){
                            leftTupleInterval->constrainUpper(idVec, strict);
                            rightTupleInterval->constrainLower(leftLowerVec, strict);
                        } else {
                            leftTupleInterval->constrainLower(idVec, strict);
                            rightTupleInterval->constrainUpper(leftLowerVec, strict);
                        }
                        
                    } else {
                        std::vector<uint32_t> rightUpperVec(rightIds.begin(), rightIds.begin() + leftTupleInterval->tupleSize());
                        
                        auto idVec = leftIds;
                        expandIdVec(&varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR->operator[](index), idVec, rightTupleInterval->tupleSize(), index + varPositionsL->operator[](index)->colorType->productSize());

                        if(lessthan){
                            leftTupleInterval->constrainUpper(rightUpperVec, strict);
                            rightTupleInterval->constrainLower(idVec, strict);
                        } else {
                            leftTupleInterval->constrainLower(rightUpperVec, strict);
                            rightTupleInterval->constrainUpper(idVec, strict);
                        }                            
                    }
                }               
            }

            void restrictEqDiagonal(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index){
                diagonalVars.insert(var);
                diagonalVars.insert(varPositionsR->operator[](index));

                for(auto& varMap : variableMap){
                    auto leftTupleIntervalVal = varMap[var];
                    auto rightTupleIntervalVal = varMap[varPositionsR->operator[](index)];
                    auto leftTupleInterval = &varMap[var];
                    auto rightTupleInterval = &varMap[varPositionsR->operator[](index)];
                    int32_t leftVarModifier = getVarModifier(&varModifierMapL->operator[](var).back(), index);
                    int32_t rightVarModifier = getVarModifier(&varModifierMapR->operator[](varPositionsR->operator[](index)).back(), index);

                    leftTupleIntervalVal.applyModifier(-leftVarModifier, var->colorType->getConstituentsSizes());
                    rightTupleIntervalVal.applyModifier(-rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());
                    //comparing vars of same size
                    if(var->colorType->productSize() == varPositionsR->operator[](index)->colorType->productSize()){
                        Colored::intervalTuple_t newIntervalTuple = getIntervalOverlap(&leftTupleIntervalVal._intervals, &rightTupleIntervalVal._intervals);
                    
                        *leftTupleInterval = newIntervalTuple;
                        *rightTupleInterval = newIntervalTuple;
                        leftTupleInterval->applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                        rightTupleInterval->applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());
                    } else if(var->colorType->productSize() > varPositionsR->operator[](index)->colorType->productSize()){
                        std::vector<Colored::interval_t> resizedLeftIntervals = leftTupleIntervalVal.shrinkIntervals(varPositionsR->operator[](index)->colorType->productSize());
                        auto intervalVec = rightTupleIntervalVal._intervals;

                        expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, var, intervalVec, leftTupleInterval->tupleSize(), index + varPositionsR->operator[](index)->colorType->productSize());

                        Colored::intervalTuple_t newIntervalTupleR = getIntervalOverlap(&rightTupleIntervalVal._intervals, &resizedLeftIntervals);
                        Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(&leftTupleIntervalVal._intervals, &intervalVec);

                        newIntervalTupleL.applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                        newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                        *leftTupleInterval = newIntervalTupleL;
                        *rightTupleInterval = newIntervalTupleR;
                    } else {
                        std::vector<Colored::interval_t> resizedRightIntervals = rightTupleIntervalVal.shrinkIntervals(varPositionsL->operator[](index)->colorType->productSize());
                        auto intervalVec = leftTupleIntervalVal._intervals;

                        expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR->operator[](index), intervalVec, rightTupleInterval->tupleSize(), index + varPositionsL->operator[](index)->colorType->productSize());

                        Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(&leftTupleIntervalVal._intervals, &resizedRightIntervals);
                        Colored::intervalTuple_t newIntervalTupleR = getIntervalOverlap(&rightTupleIntervalVal._intervals, &intervalVec);

                        newIntervalTupleL.applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                        newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                        *leftTupleInterval = newIntervalTupleL;
                        *rightTupleInterval = newIntervalTupleR;
                    }
                }
            }


            void restrictVars(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars, 
                            bool lessthan, bool strict) {


                for(auto varPositionPair : *varPositionsL){
                    uint32_t index = varPositionPair.first;
                    if(varPositionsR->count(index)){
                        restrictDiagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, 
                                        constantMapL, constantMapR, diagonalVars, varPositionPair.second, index, lessthan, strict);
                    } else {
                        restrictByConstant(variableMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, 
                                        varPositionPair.second, varPositionPair.second, index, lessthan, strict);                        
                    }
                }

                for(auto varPositionPair : *varPositionsR){
                    uint32_t index = varPositionPair.first;

                    if(constantMapL->count(index)){
                        restrictByConstant(variableMap,varModifierMapL, varModifierMapR, varPositionsL, constantMapL, 
                                        varPositionPair.second, varPositionsR->operator[](index),index, lessthan, strict);                       
                    }
                }
                
            }

            void restrictEquality(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> *varPositionsR,
                            std::unordered_map<uint32_t, const Color*> *constantMapL,
                            std::unordered_map<uint32_t, const Color*> *constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars) {
                for(auto varPositionPair : *varPositionsL){
                    uint32_t index = varPositionPair.first;
                    if(varPositionsR->count(index)){
                        restrictEqDiagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, 
                                    constantMapL, constantMapR, diagonalVars, varPositionPair.second, index);
                    } else {
                        restrictEqByConstant(variableMap, varModifierMapR, varModifierMapL, varPositionsR,
                                    constantMapR, varPositionPair.second, index);
                    }
                }

                for(auto varPositionPair : *varPositionsR){
                    uint32_t index = varPositionPair.first;

                    if(constantMapL->count(index)){
                        restrictEqByConstant(variableMap, varModifierMapL, varModifierMapR, varPositionsL, 
                                    constantMapL, varPositionPair.second, index);
                    }
                }                
            }

            Colored::intervalTuple_t shiftIntervals(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t>& varMap, std::vector<const Colored::ColorType *> *colortypes, PetriEngine::Colored::intervalTuple_t *intervals, int32_t modifier, uint32_t ctSizeBefore) const {
                Colored::intervalTuple_t newIntervals;

                for(uint32_t i = 0;  i < intervals->size(); i++) {
                    Colored::interval_t newInterval;
                    std::vector<Colored::interval_t> tempIntervals;
                    auto interval = &intervals->operator[](i);
                    for(uint32_t j = 0; j < interval->_ranges.size(); j++) {
                        auto& range = interval->operator[](j);
                        size_t ctSize = colortypes->operator[](j+ ctSizeBefore)->size();
                        
                        auto shiftedInterval = newIntervals.shiftInterval(range._lower, range._upper, ctSize, modifier);
                        range._lower = shiftedInterval.first;
                        range._upper = shiftedInterval.second;


                        if(range._upper+1 == range._lower){
                            if(tempIntervals.empty()){
                                newInterval.addRange(0, ctSize-1);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for (auto& tempInterval : tempIntervals){ 
                                    tempInterval.addRange(0, ctSize-1);
                                }
                            }
                        } else if(range._upper < range._lower ){
                            
                            if(tempIntervals.empty()){
                                auto intervalCopy = newInterval;
                                newInterval.addRange(range._lower, ctSize-1);
                                intervalCopy.addRange(0,range._upper);
                                tempIntervals.push_back(newInterval);
                                tempIntervals.push_back(intervalCopy);
                            } else {
                                std::vector<Colored::interval_t> newTempIntervals;
                                for(auto tempInterval : tempIntervals){
                                    auto intervalCopy = tempInterval;
                                    tempInterval.addRange(range._lower, ctSize-1);
                                    intervalCopy.addRange(0,range._upper);
                                    newTempIntervals.push_back(intervalCopy);
                                    newTempIntervals.push_back(tempInterval);
                                }
                                tempIntervals = newTempIntervals;
                            }                            
                        } else {
                            if(tempIntervals.empty()){
                                newInterval.addRange(range);
                                tempIntervals.push_back(newInterval);
                            } else {
                                for (auto& tempInterval : tempIntervals){ 
                                    tempInterval.addRange(range);
                                }
                            }
                        }
                    }
                    for(auto tempInterval : tempIntervals){
                        newIntervals.addInterval(tempInterval);
                    }                   
                }
                return newIntervals;
            }
        };        
    }
}


#endif /* GuardRestrictions_H */