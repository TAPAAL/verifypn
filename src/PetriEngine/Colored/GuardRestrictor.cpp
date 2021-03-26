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

#include "PetriEngine/Colored/GuardRestrictor.h"

namespace PetriEngine{
    namespace Colored{
        
        GuardRestrictor::GuardRestrictor(){}

        int32_t GuardRestrictor::getVarModifier(std::unordered_map<uint32_t, int32_t> *modPairMap, uint32_t index){
            int32_t modifier;
            for(auto idModPair : *modPairMap){
                if(idModPair.first == index){
                    modifier = idModPair.second;
                    break;
                }
            }
            return modifier;
        }

        interval_t GuardRestrictor::getIntervalFromIds(std::vector<uint32_t> *idVec, uint32_t ctSize, int32_t modifier){
            interval_t interval;
            for(auto id : *idVec){          
                int32_t val = ctSize + (id + modifier);
                auto colorVal = val % ctSize;                  
                interval.addRange(colorVal,colorVal);
            }
            return interval; 
        }

        intervalTuple_t GuardRestrictor::getIntervalOverlap(std::vector<interval_t> *intervals1, std::vector<interval_t> *intervals2){
            intervalTuple_t newIntervalTuple;
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

        void GuardRestrictor::expandIdVec(std::unordered_map<const Variable *, intervalTuple_t> *varMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                        std::unordered_map<uint32_t, const Variable *> *varPositions,
                        std::unordered_map<uint32_t, const Color*> *constantMap,
                        const Variable *otherVar, 
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

        void GuardRestrictor::expandIntervalVec(std::unordered_map<const Variable *, intervalTuple_t> *varMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *mainVarModifierMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *otherVarModifierMap,
                        std::unordered_map<uint32_t, const Variable *> *varPositions,
                        std::unordered_map<uint32_t, const Color*> *constantMap,
                        const Variable *otherVar, 
                        std::vector<interval_t> &intervalVec, size_t targetSize, uint32_t index){

            while(intervalVec.size() < targetSize){
                if(varPositions->count(index)){
                    auto rightTupleInterval = varMap->operator[](varPositions->operator[](index));
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

        void GuardRestrictor::restrictDiagonal(std::vector<std::unordered_map<const Variable *, intervalTuple_t>> *variableMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                        std::unordered_map<uint32_t, const Variable *> *varPositionsL,
                        std::unordered_map<uint32_t, const Variable *> *varPositionsR,
                        std::unordered_map<uint32_t, const Color*> *constantMapL,
                        std::unordered_map<uint32_t, const Color*> *constantMapR, 
                        bool lessthan, bool strict) {

            for(auto& varMap : *variableMap){
                for(auto varPositionPair : *varPositionsL){
                    uint32_t index = varPositionPair.first;
                    if(varPositionsR->count(index)){
                        if(varMap.count(varPositionPair.second) == 0){
                            std::cout << "Unable to find left var " << varPositionPair.second->name << std::endl;
                        }
                        if(varMap.count(varPositionsR->operator[](index)) == 0){
                            std::cout << "Unable to find right var " << varPositionsR->operator[](index)->name << std::endl;
                        }
                        auto leftTupleInterval = &varMap[varPositionPair.second];
                        auto rightTupleInterval = &varMap[varPositionsR->operator[](index)];
                        int32_t leftVarModifier = getVarModifier(&varModifierMapL->operator[](varPositionPair.second).back(), index);
                        int32_t rightVarModifier = getVarModifier(&varModifierMapR->operator[](varPositionsR->operator[](index)).back(), index);

                        auto leftIds = leftTupleInterval->getLowerIds(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                        auto rightIds = rightTupleInterval->getUpperIds(-rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                        //comparing vars of same size
                        if(varPositionPair.second->colorType->productSize() == varPositionsR->operator[](index)->colorType->productSize()){
                            if(lessthan){
                                leftTupleInterval->constrainUpper(rightIds, strict);
                                rightTupleInterval->constrainLower(leftIds, strict);
                            } else {
                                leftTupleInterval->constrainLower(rightIds, strict);
                                rightTupleInterval->constrainUpper(leftIds, strict);
                            }
                            
                        } else if(varPositionPair.second->colorType->productSize() > varPositionsR->operator[](index)->colorType->productSize()){
                            std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                            
                            auto idVec = rightIds;

                            expandIdVec(&varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, idVec, leftTupleInterval->tupleSize(), index + varPositionsR->operator[](index)->colorType->productSize());

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
                    } else {
                        auto rightColor = constantMapR->operator[](index);
                        auto leftTupleInterval = &varMap[varPositionPair.second];
                        std::vector<uint32_t> idVec;
                        rightColor->getTupleId(&idVec);

                        expandIdVec(&varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, idVec, leftTupleInterval->tupleSize(), index + idVec.size());

                        if(lessthan){
                            leftTupleInterval->constrainUpper(idVec, strict);
                        } else {
                            leftTupleInterval->constrainLower(idVec, strict);
                        }                        
                    }
                }

                for(auto varPositionPair : *varPositionsR){
                    uint32_t index = varPositionPair.first;

                    if(constantMapL->count(index)){
                        auto leftColor = constantMapL->operator[](index);
                        auto rightTupleInterval = &varMap[varPositionPair.second];
                        std::vector<uint32_t> idVec;
                        leftColor->getTupleId(&idVec);

                        expandIdVec(&varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR->operator[](index), idVec, rightTupleInterval->tupleSize(), index + idVec.size());

                        if(lessthan){
                            rightTupleInterval->constrainUpper(idVec, strict);
                        } else {
                            rightTupleInterval->constrainLower(idVec, strict);
                        }                        
                    }
                }
            }
        }

        void GuardRestrictor::restrictEquality(std::vector<std::unordered_map<const Variable *, intervalTuple_t>> *variableMap,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapL,
                        std::unordered_map<const Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> *varModifierMapR,
                        std::unordered_map<uint32_t, const Variable *> *varPositionsL,
                        std::unordered_map<uint32_t, const Variable *> *varPositionsR,
                        std::unordered_map<uint32_t, const Color*> *constantMapL,
                        std::unordered_map<uint32_t, const Color*> *constantMapR) {
            
            for(auto& varMap : *variableMap){
                for(auto varPositionPair : *varPositionsL){
                    uint32_t index = varPositionPair.first;
                    if(varPositionsR->count(index)){
                        auto leftTupleIntervalVal = varMap[varPositionPair.second];
                        auto rightTupleIntervalVal = varMap[varPositionsR->operator[](index)];
                        auto leftTupleInterval = &varMap[varPositionPair.second];
                        auto rightTupleInterval = &varMap[varPositionsR->operator[](index)];
                        int32_t leftVarModifier = getVarModifier(&varModifierMapL->operator[](varPositionPair.second).back(), index);
                        int32_t rightVarModifier = getVarModifier(&varModifierMapR->operator[](varPositionsR->operator[](index)).back(), index);

                        leftTupleIntervalVal.applyModifier(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                        rightTupleIntervalVal.applyModifier(-rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());
                        //comparing vars of same size
                        if(varPositionPair.second->colorType->productSize() == varPositionsR->operator[](index)->colorType->productSize()){
                            intervalTuple_t newIntervalTuple = getIntervalOverlap(&leftTupleIntervalVal._intervals, &rightTupleIntervalVal._intervals);

                            *leftTupleInterval = newIntervalTuple;
                            *rightTupleInterval = newIntervalTuple;
                            leftTupleInterval->applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            rightTupleInterval->applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());
                        } else if(varPositionPair.second->colorType->productSize() > varPositionsR->operator[](index)->colorType->productSize()){
                            std::vector<interval_t> resizedLeftIntervals = leftTupleIntervalVal.shrinkIntervals(varPositionsR->operator[](index)->colorType->productSize());
                            auto intervalVec = rightTupleIntervalVal._intervals;

                            expandIntervalVec(&varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, intervalVec, leftTupleInterval->tupleSize(), index + varPositionsR->operator[](index)->colorType->productSize());

                            intervalTuple_t newIntervalTupleR = getIntervalOverlap(&rightTupleIntervalVal._intervals, &resizedLeftIntervals);
                            intervalTuple_t newIntervalTupleL = getIntervalOverlap(&leftTupleIntervalVal._intervals, &intervalVec);

                            newIntervalTupleL.applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                            *leftTupleInterval = newIntervalTupleL;
                            *rightTupleInterval = newIntervalTupleR;
                        } else {
                            std::vector<interval_t> resizedRightIntervals = rightTupleIntervalVal.shrinkIntervals(varPositionsL->operator[](index)->colorType->productSize());
                            auto intervalVec = leftTupleIntervalVal._intervals;

                            expandIntervalVec(&varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR->operator[](index), intervalVec, rightTupleInterval->tupleSize(), index + varPositionsL->operator[](index)->colorType->productSize());

                            intervalTuple_t newIntervalTupleL = getIntervalOverlap(&leftTupleIntervalVal._intervals, &resizedRightIntervals);
                            intervalTuple_t newIntervalTupleR = getIntervalOverlap(&rightTupleIntervalVal._intervals, &intervalVec);

                            newIntervalTupleL.applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR->operator[](index)->colorType->getConstituentsSizes());

                            *leftTupleInterval = newIntervalTupleL;
                            *rightTupleInterval = newIntervalTupleR;
                        }
                    } else {
                        auto rightColor = constantMapR->operator[](index);
                        auto leftTupleInterval = &varMap[varPositionPair.second];
                        std::vector<uint32_t> idVec;
                        rightColor->getTupleId(&idVec);
                        int32_t leftVarModifier = getVarModifier(&varModifierMapL->operator[](varPositionPair.second).back(), index);

                        std::vector<interval_t> intervals;
                        intervals.push_back(getIntervalFromIds(&idVec, varPositionPair.second->colorType->size(), leftVarModifier));

                        expandIntervalVec(&varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, intervals, leftTupleInterval->tupleSize(), index + idVec.size());

                        intervalTuple_t newIntervalTupleL = getIntervalOverlap(&leftTupleInterval->_intervals, &intervals);
                        *leftTupleInterval = newIntervalTupleL;
                    }
                }

                for(auto varPositionPair : *varPositionsR){
                    uint32_t index = varPositionPair.first;

                    if(constantMapL->count(index)){
                        auto leftColor = constantMapL->operator[](index);
                        auto rightTupleInterval = &varMap[varPositionPair.second];
                        std::vector<uint32_t> idVec;
                        leftColor->getTupleId(&idVec);
                        int32_t rightVarModifier = getVarModifier(&varModifierMapR->operator[](varPositionPair.second).back(), index);

                        std::vector<interval_t> intervals;
                        intervals.push_back(getIntervalFromIds(&idVec, varPositionPair.second->colorType->size(), rightVarModifier));

                        expandIntervalVec(&varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR->operator[](index), intervals, rightTupleInterval->tupleSize(), index + idVec.size());

                        intervalTuple_t newIntervalTupleR = getIntervalOverlap(&rightTupleInterval->_intervals, &intervals);
                        *rightTupleInterval = newIntervalTupleR;
                    }
                }
            }
        }

        intervalTuple_t GuardRestrictor::shiftIntervals(std::vector<const ColorType *> *colortypes, intervalTuple_t *intervals, int32_t modifier, uint32_t ctSizeBefore) const {
            intervalTuple_t newIntervals;

            for(uint32_t i = 0;  i < intervals->size(); i++) {
                interval_t newInterval;
                std::vector<interval_t> tempIntervals;
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
                            std::vector<interval_t> newTempIntervals;
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
    }
} 
