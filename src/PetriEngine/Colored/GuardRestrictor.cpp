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

        int32_t GuardRestrictor::getVarModifier(const std::unordered_map<uint32_t, int32_t> &modPairMap, uint32_t index) const{
            int32_t modifier;
            for(auto idModPair : modPairMap){
                if(idModPair.first == index){
                    modifier = idModPair.second;
                    break;
                }
            }
            return modifier;
        }

        interval_t GuardRestrictor::getIntervalFromIds(const std::vector<uint32_t> &idVec, uint32_t ctSize, int32_t modifier) const{
            interval_t interval;
            for(auto id : idVec){          
                int32_t val = ctSize + (id + modifier);
                auto colorVal = val % ctSize;                  
                interval.addRange(colorVal,colorVal);
            }
            return interval; 
        }

        interval_vector_t GuardRestrictor::getIntervalOverlap(const interval_vector_t &intervals1, const interval_vector_t &intervals2) const{
            interval_vector_t newIntervalTuple;
            for(const auto &mainInterval : intervals1){
                for(const auto &otherInterval : intervals2){
                    auto intervalOverlap = otherInterval.getOverlap(mainInterval);

                    if(intervalOverlap.isSound()){
                        newIntervalTuple.addInterval(std::move(intervalOverlap));
                    }
                }
            }
            return newIntervalTuple;
        }

        void GuardRestrictor::expandIdVec(const VariableIntervalMap &varMap,
                        const VariableModifierMap &mainVarModifierMap,
                        const VariableModifierMap &otherVarModifierMap,
                        const PositionVariableMap &varPositions,
                        const std::unordered_map<uint32_t, const Color*> &constantMap,
                        const Variable *otherVar, 
                        std::vector<uint32_t> &idVec, size_t targetSize, uint32_t index) const{
            while(idVec.size() < targetSize){
                if(varPositions.count(index)){
                    const auto &rightTupleInterval = varMap.find(varPositions.find(index)->second)->second;
                    int32_t rightVarMod = getVarModifier(mainVarModifierMap.find(varPositions.find(index)->second)->second.back(), index);
                    auto ids = rightTupleInterval.getUpperIds(-rightVarMod, varPositions.find(index)->second->colorType->getConstituentsSizes());
                    idVec.insert(idVec.end(), ids.begin(), ids.end());
                    index += varPositions.find(index)->second->colorType->productSize();
                } else {
                    auto oldSize = idVec.size();
                    constantMap.find(index)->second->getTupleId(idVec); 
                    int32_t leftVarMod = getVarModifier(otherVarModifierMap.find(otherVar)->second.back(), index);

                    for(auto& id : idVec){
                        id = (otherVar->colorType->size()+(id + leftVarMod)) % otherVar->colorType->size();
                    }                                   
                    index+= idVec.size() - oldSize;
                }
            }
        }

        void GuardRestrictor::expandIntervalVec(const VariableIntervalMap &varMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const PositionVariableMap &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Colored::Variable *otherVar, 
                            Colored::interval_vector_t &intervalVec, size_t targetSize, uint32_t index) const {
            while(intervalVec.size() < targetSize){
                if(varPositions.count(index)){
                    auto rightTupleInterval = varMap.find(varPositions.find(index)->second)->second;
                    int32_t rightVarMod = getVarModifier(mainVarModifierMap.find(varPositions.find(index)->second)->second.back(), index);
                    rightTupleInterval.applyModifier(-rightVarMod, varPositions.find(index)->second->colorType->getConstituentsSizes());
                    intervalVec.append(rightTupleInterval);
                    index += varPositions.find(index)->second->colorType->productSize();
                } else {
                    std::vector<uint32_t> colorIdVec;
                    constantMap.find(index)->second->getTupleId(colorIdVec);
                    int32_t leftVarModifier = getVarModifier(otherVarModifierMap.find(otherVar)->second.back(), index);

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

        void GuardRestrictor::restrictByConstant(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const PositionVariableMap &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Colored::Variable *var,
                            const Colored::Variable *otherVar, 
                            uint32_t index, bool lessthan, bool strict) const{
            for(auto& varMap : variableMap){
                auto leftColor = constantMap.find(index)->second;
                auto &rightTupleInterval = varMap.find(var)->second;
                std::vector<uint32_t> idVec;
                leftColor->getTupleId(idVec);

                expandIdVec(varMap, mainVarModifierMap, otherVarModifierMap, varPositions, constantMap, otherVar, idVec, rightTupleInterval.tupleSize(), index + idVec.size());

                if(lessthan){
                    rightTupleInterval.constrainUpper(idVec, strict);
                } else {
                    rightTupleInterval.constrainLower(idVec, strict);
                }
            }                 
        }

        void GuardRestrictor::restrictEqByConstant(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const PositionVariableMap &varPositions,
                            const std::unordered_map<uint32_t, const Color*> &constantMap,
                            const Colored::Variable *var,
                            uint32_t index) const{
            for(auto& varMap : variableMap){
                auto color = constantMap.find(index)->second;
                auto &tupleInterval = varMap.find(var)->second;
                std::vector<uint32_t> idVec;
                color->getTupleId(idVec);
                int32_t varModifier = getVarModifier(otherVarModifierMap.find(var)->second.back(), index);

                
                std::vector<Colored::interval_t> iv{getIntervalFromIds(idVec, var->colorType->size(), varModifier)};
                Colored::interval_vector_t intervals(iv);

                expandIntervalVec(varMap, mainVarModifierMap, otherVarModifierMap, varPositions, constantMap, var, intervals, tupleInterval.tupleSize(), index + idVec.size());

                auto newIntervalTupleL = getIntervalOverlap(tupleInterval, intervals);
                tupleInterval = newIntervalTupleL;
            }
        }

        void GuardRestrictor::restrictDiagonal(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index, bool lessthan, bool strict) const{

            diagonalVars.insert(var);
            diagonalVars.insert(varPositionsR.find(index)->second);

            for(auto& varMap : variableMap){
                if(varMap.count(var) == 0){
                    std::cout << "Unable to find left var " << var->name << std::endl;
                }
                if(varMap.count(varPositionsR.find(index)->second) == 0){
                    std::cout << "Unable to find right var " << varPositionsR.find(index)->second->name << std::endl;
                }

                auto &leftTupleInterval = varMap[var];
                auto &rightTupleInterval = varMap[varPositionsR.find(index)->second];
                int32_t leftVarModifier = getVarModifier(varModifierMapL.find(var)->second.back(), index);
                int32_t rightVarModifier = getVarModifier(varModifierMapR.find(varPositionsR.find(index)->second)->second.back(), index);

                const auto &leftIds = leftTupleInterval.getLowerIds(-leftVarModifier, var->colorType->getConstituentsSizes());
                const auto &rightIds = rightTupleInterval.getUpperIds(-rightVarModifier, varPositionsR.find(index)->second->colorType->getConstituentsSizes());

                //comparing vars of same size
                if(var->colorType->productSize() == varPositionsR.find(index)->second->colorType->productSize()){
                    if(lessthan){
                        leftTupleInterval.constrainUpper(rightIds, strict);
                        rightTupleInterval.constrainLower(leftIds, strict);
                    } else {
                        leftTupleInterval.constrainLower(rightIds, strict);
                        rightTupleInterval.constrainUpper(leftIds, strict);
                    }
                } else if(var->colorType->productSize() > varPositionsR.find(index)->second->colorType->productSize()){
                    std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval.tupleSize());
                    
                    auto idVec = rightIds;

                    expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, var, idVec, leftTupleInterval.tupleSize(), index + varPositionsR.find(index)->second->colorType->productSize());

                    if(lessthan){
                        leftTupleInterval.constrainUpper(idVec, strict);
                        rightTupleInterval.constrainLower(leftLowerVec, strict);
                    } else {
                        leftTupleInterval.constrainLower(idVec, strict);
                        rightTupleInterval.constrainUpper(leftLowerVec, strict);
                    }
                    
                } else {
                    std::vector<uint32_t> rightUpperVec(rightIds.begin(), rightIds.begin() + leftTupleInterval.tupleSize());
                    
                    auto idVec = leftIds;
                    expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR.find(index)->second, idVec, rightTupleInterval.tupleSize(), index + varPositionsL.find(index)->second->colorType->productSize());

                    if(lessthan){
                        leftTupleInterval.constrainUpper(rightUpperVec, strict);
                        rightTupleInterval.constrainLower(idVec, strict);
                    } else {
                        leftTupleInterval.constrainLower(rightUpperVec, strict);
                        rightTupleInterval.constrainUpper(idVec, strict);
                    }                            
                }
            }               
        }

        void GuardRestrictor::restrictEqDiagonal(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars,
                            const Colored::Variable *var, 
                            uint32_t index) const{
            diagonalVars.insert(var);
            diagonalVars.insert(varPositionsR.find(index)->second);

            for(auto& varMap : variableMap){
                auto leftTupleIntervalVal = varMap.find(var)->second;
                auto rightTupleIntervalVal = varMap.find(varPositionsR.find(index)->second)->second;
                auto &leftTupleInterval = varMap.find(var)->second;
                auto &rightTupleInterval = varMap.find(varPositionsR.find(index)->second)->second;
                int32_t leftVarModifier = getVarModifier(varModifierMapL.find(var)->second.back(), index);
                int32_t rightVarModifier = getVarModifier(varModifierMapR.find(varPositionsR.find(index)->second)->second.back(), index);

                leftTupleIntervalVal.applyModifier(-leftVarModifier, var->colorType->getConstituentsSizes());
                rightTupleIntervalVal.applyModifier(-rightVarModifier, varPositionsR.find(index)->second->colorType->getConstituentsSizes());
                //comparing vars of same size
                if(var->colorType->productSize() == varPositionsR.find(index)->second->colorType->productSize()){
                    Colored::interval_vector_t newIntervalTuple = getIntervalOverlap(leftTupleIntervalVal, rightTupleIntervalVal);
                
                    leftTupleInterval = newIntervalTuple;
                    rightTupleInterval = newIntervalTuple;
                    leftTupleInterval.applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                    rightTupleInterval.applyModifier(rightVarModifier, varPositionsR.find(index)->second->colorType->getConstituentsSizes());
                } else if(var->colorType->productSize() > varPositionsR.find(index)->second->colorType->productSize()){
                    const std::vector<Colored::interval_t> &resizedLeftIntervals = leftTupleIntervalVal.shrinkIntervals(varPositionsR.find(index)->second->colorType->productSize());
                    auto intervalVec = rightTupleIntervalVal;

                    expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, 
                        var, intervalVec, leftTupleInterval.tupleSize(), 
                        index + varPositionsR.find(index)->second->colorType->productSize());

                    Colored::interval_vector_t newIntervalTupleR = getIntervalOverlap(rightTupleIntervalVal, resizedLeftIntervals);
                    Colored::interval_vector_t newIntervalTupleL = getIntervalOverlap(leftTupleIntervalVal, intervalVec);

                    newIntervalTupleL.applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                    newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR.find(index)->second->colorType->getConstituentsSizes());

                    leftTupleInterval = newIntervalTupleL;
                    rightTupleInterval = newIntervalTupleR;
                } else {
                    std::vector<Colored::interval_t> resizedRightIntervals = rightTupleIntervalVal.shrinkIntervals(varPositionsL.find(index)->second->colorType->productSize());
                    auto intervalVec = leftTupleIntervalVal;

                    expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR.find(index)->second, intervalVec, rightTupleInterval.tupleSize(), index + varPositionsL.find(index)->second->colorType->productSize());

                    Colored::interval_vector_t newIntervalTupleL = getIntervalOverlap(leftTupleIntervalVal, resizedRightIntervals);
                    Colored::interval_vector_t newIntervalTupleR = getIntervalOverlap(rightTupleIntervalVal, intervalVec);

                    newIntervalTupleL.applyModifier(leftVarModifier, var->colorType->getConstituentsSizes());
                    newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR.find(index)->second->colorType->getConstituentsSizes());

                    leftTupleInterval = newIntervalTupleL;
                    rightTupleInterval = newIntervalTupleR;
                }
            }
        }

        void GuardRestrictor::restrictVars(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars, 
                            bool lessthan, bool strict) const{

            for(const auto &varPositionPair : varPositionsL){
                uint32_t index = varPositionPair.first;
                if(varPositionsR.count(index)){
                    restrictDiagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, 
                                    constantMapL, constantMapR, diagonalVars, varPositionPair.second, index, lessthan, strict);
                } else {
                    restrictByConstant(variableMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, 
                                    varPositionPair.second, varPositionPair.second, index, lessthan, strict);                        
                }
            }

            for(const auto &varPositionPair : varPositionsR){
                uint32_t index = varPositionPair.first;

                if(constantMapL.count(index)){
                    restrictByConstant(variableMap,varModifierMapL, varModifierMapR, varPositionsL, constantMapL, 
                                    varPositionPair.second, varPositionsR.find(index)->second,index, lessthan, strict);                       
                }
            }            
        }

        void GuardRestrictor::restrictEquality(std::vector<VariableIntervalMap>& variableMap,
                            const VariableModifierMap &varModifierMapL,
                            const VariableModifierMap &varModifierMapR,
                            const PositionVariableMap &varPositionsL,
                            const PositionVariableMap &varPositionsR,
                            const std::unordered_map<uint32_t, const Color*> &constantMapL,
                            const std::unordered_map<uint32_t, const Color*> &constantMapR,
                            std::set<const Colored::Variable*> &diagonalVars) const {
            for(const auto& varPositionPair : varPositionsL){
                uint32_t index = varPositionPair.first;
                if(varPositionsR.count(index)){
                    restrictEqDiagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR, 
                                constantMapL, constantMapR, diagonalVars, varPositionPair.second, index);
                } else {
                    restrictEqByConstant(variableMap, varModifierMapR, varModifierMapL, varPositionsR,
                                constantMapR, varPositionPair.second, index);
                }
            }

            for(const auto &varPositionPair : varPositionsR){
                uint32_t index = varPositionPair.first;

                if(constantMapL.count(index)){
                    restrictEqByConstant(variableMap, varModifierMapL, varModifierMapR, varPositionsL, 
                                constantMapL, varPositionPair.second, index);
                }
            }                
        }

        interval_vector_t GuardRestrictor::shiftIntervals(const VariableIntervalMap& varMap, const std::vector<const Colored::ColorType *> &colortypes, PetriEngine::Colored::interval_vector_t &intervals, int32_t modifier, uint32_t ctSizeBefore) const {
            Colored::interval_vector_t newIntervals;
            for(uint32_t i = 0;  i < intervals.size(); i++) {
                Colored::interval_t newInterval;
                std::vector<Colored::interval_t> tempIntervals;
                auto &interval = intervals[i];
                for(uint32_t j = 0; j < interval._ranges.size(); j++) {
                    auto& range = interval.operator[](j);
                    size_t ctSize = colortypes[j+ ctSizeBefore]->size();
                    
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
                for(const auto& tempInterval : tempIntervals){
                    newIntervals.addInterval(tempInterval);
                }                   
            }
            return newIntervals;
        }       
    }
} 
