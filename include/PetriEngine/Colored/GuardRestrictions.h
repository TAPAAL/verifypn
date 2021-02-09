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

            int32_t getVarModifier(std::unordered_map<uint32_t, int32_t> modPairMap, uint32_t index){
                int32_t modifier;
                for(auto idModPair : modPairMap){
                    if(idModPair.first == index){
                        modifier = idModPair.second;
                        break;
                    }
                }
                return modifier;
            }

            Colored::interval_t getIntervalFromIds(std::vector<uint32_t> idVec, uint32_t ctSize, int32_t modifier){
                Colored::interval_t interval;
                for(auto id : idVec){          
                    int32_t val = ctSize + (id + modifier);
                    auto colorVal = val % ctSize;                  
                    interval.addRange(colorVal,colorVal);
                }
                return interval; 
            }

            Colored::intervalTuple_t getIntervalOverlap(std::vector<Colored::interval_t> intervals1, std::vector<Colored::interval_t> intervals2){
                Colored::intervalTuple_t newIntervalTuple;
                for(auto mainInterval : intervals1){
                    for(auto otherInterval : intervals2){
                        auto intervalOverlap = otherInterval.getOverlap(mainInterval);

                        if(intervalOverlap.isSound()){
                            newIntervalTuple.addInterval(intervalOverlap);
                        }
                    }
                }
                return newIntervalTuple;
            }

            void expandIdVec(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t> varMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositions,
                            std::unordered_map<uint32_t, const Color*> constantMap,
                            const Colored::Variable *otherVar, 
                            std::vector<uint32_t> &idVec, size_t targetSize, uint32_t index){
                while(idVec.size() < targetSize){
                    if(varPositions.count(index)){
                        auto rightTupleInterval = &varMap[varPositions[index]];
                        int32_t rightVarMod = getVarModifier(mainVarModifierMap[varPositions[index]].back(), index);
                        auto ids = rightTupleInterval->getUpperIds(-rightVarMod, varPositions[index]->colorType->getConstituentsSizes());
                        idVec.insert(idVec.end(), ids.begin(), ids.end());
                        index += varPositions[index]->colorType->productSize();
                    } else {
                        auto oldSize = idVec.size();
                        constantMap[index]->getTupleId(&idVec); 
                        int32_t leftVarMod = getVarModifier(otherVarModifierMap[otherVar].back(), index);

                        for(auto& id : idVec){
                            id = (otherVar->colorType->size()+(id + leftVarMod)) % otherVar->colorType->size();
                        }                                   
                        index+= idVec.size() - oldSize;
                    }
                }
            }

            void expandIntervalVec(std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::intervalTuple_t> varMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> mainVarModifierMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> otherVarModifierMap,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositions,
                            std::unordered_map<uint32_t, const Color*> constantMap,
                            const Colored::Variable *otherVar, 
                            std::vector<Colored::interval_t> &intervalVec, size_t targetSize, uint32_t index){

                while(intervalVec.size() < targetSize){
                    if(varPositions.count(index)){
                        auto rightTupleInterval = varMap[varPositions[index]];
                        int32_t rightVarMod = getVarModifier(mainVarModifierMap[varPositions[index]].back(), index);
                        rightTupleInterval.applyModifier(-rightVarMod, varPositions[index]->colorType->getConstituentsSizes());
                        intervalVec.insert(intervalVec.end(), rightTupleInterval._intervals.begin(), rightTupleInterval._intervals.end());
                        index += varPositions[index]->colorType->productSize();
                    } else {
                        std::vector<uint32_t> colorIdVec;
                        constantMap[index]->getTupleId(&colorIdVec);
                        int32_t leftVarModifier = getVarModifier(otherVarModifierMap[otherVar].back(), index);

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

            void restrictDiagonal(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR,
                            std::unordered_map<uint32_t, const Color*> constantMapL,
                            std::unordered_map<uint32_t, const Color*> constantMapR, 
                            bool lessthan, bool strict) {

                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            if(varMap.count(varPositionPair.second) == 0){
                                std::cout << "Unable to find left var " << varPositionPair.second->name << std::endl;
                            }
                            if(varMap.count(varPositionsR[index]) == 0){
                                std::cout << "Unable to find right var " << varPositionsR[index]->name << std::endl;
                            }
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier = getVarModifier(varModifierMapL[varPositionPair.second].back(), index);
                            int32_t rightVarModifier = getVarModifier(varModifierMapR[varPositionsR[index]].back(), index);

                            auto leftIds = leftTupleInterval->getLowerIds(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            auto rightIds = rightTupleInterval->getUpperIds(-rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){
                                if(lessthan){
                                    leftTupleInterval->constrainUpper(rightIds, strict);
                                    rightTupleInterval->constrainLower(leftIds, strict);
                                } else {
                                    leftTupleInterval->constrainLower(rightIds, strict);
                                    rightTupleInterval->constrainUpper(leftIds, strict);
                                }
                                
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<uint32_t> leftLowerVec(leftIds.begin(), leftIds.begin() + rightTupleInterval->tupleSize());
                                
                                auto idVec = rightIds;

                                expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, idVec, leftTupleInterval->tupleSize(), index + varPositionsR[index]->colorType->productSize());

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
                                expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR[index], idVec, rightTupleInterval->tupleSize(), index + varPositionsL[index]->colorType->productSize());

                                if(lessthan){
                                    leftTupleInterval->constrainUpper(rightUpperVec, strict);
                                    rightTupleInterval->constrainLower(idVec, strict);
                                } else {
                                    leftTupleInterval->constrainLower(rightUpperVec, strict);
                                    rightTupleInterval->constrainUpper(idVec, strict);
                                }                            
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);

                            expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, idVec, leftTupleInterval->tupleSize(), index + idVec.size());

                            if(lessthan){
                                leftTupleInterval->constrainUpper(idVec, strict);
                            } else {
                                leftTupleInterval->constrainLower(idVec, strict);
                            }                        
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);

                            expandIdVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR[index], idVec, rightTupleInterval->tupleSize(), index + idVec.size());

                            if(lessthan){
                                rightTupleInterval->constrainUpper(idVec, strict);
                            } else {
                                rightTupleInterval->constrainLower(idVec, strict);
                            }                        
                        }
                    }
                }
            }

            void restrictEquality(std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>>& variableMap,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapL,
                            std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMapR,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositionsL,
                            std::unordered_map<uint32_t, const Colored::Variable *> varPositionsR,
                            std::unordered_map<uint32_t, const Color*> constantMapL,
                            std::unordered_map<uint32_t, const Color*> constantMapR) {
                
                for(auto& varMap : variableMap){
                    for(auto varPositionPair : varPositionsL){
                        uint32_t index = varPositionPair.first;
                        if(varPositionsR.count(index)){
                            auto leftTupleIntervalVal = varMap[varPositionPair.second];
                            auto rightTupleIntervalVal = varMap[varPositionsR[index]];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            auto rightTupleInterval = &varMap[varPositionsR[index]];
                            int32_t leftVarModifier = getVarModifier(varModifierMapL[varPositionPair.second].back(), index);
                            int32_t rightVarModifier = getVarModifier(varModifierMapR[varPositionsR[index]].back(), index);

                            leftTupleIntervalVal.applyModifier(-leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                            rightTupleIntervalVal.applyModifier(-rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            //comparing vars of same size
                            if(varPositionPair.second->colorType->productSize() == varPositionsR[index]->colorType->productSize()){
                                Colored::intervalTuple_t newIntervalTuple = getIntervalOverlap(leftTupleIntervalVal._intervals, rightTupleIntervalVal._intervals);
                            
                                *leftTupleInterval = newIntervalTuple;
                                *rightTupleInterval = newIntervalTuple;
                                leftTupleInterval->applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                rightTupleInterval->applyModifier(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());
                            } else if(varPositionPair.second->colorType->productSize() > varPositionsR[index]->colorType->productSize()){
                                std::vector<Colored::interval_t> resizedLeftIntervals = leftTupleIntervalVal.shrinkIntervals(varPositionsR[index]->colorType->productSize());
                                auto intervalVec = rightTupleIntervalVal._intervals;

                                expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, intervalVec, leftTupleInterval->tupleSize(), index + varPositionsR[index]->colorType->productSize());

                                Colored::intervalTuple_t newIntervalTupleR = getIntervalOverlap(rightTupleIntervalVal._intervals, resizedLeftIntervals);
                                Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(leftTupleIntervalVal._intervals, intervalVec);

                                newIntervalTupleL.applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                                *leftTupleInterval = newIntervalTupleL;
                                *rightTupleInterval = newIntervalTupleR;
                            } else {
                                std::vector<Colored::interval_t> resizedRightIntervals = rightTupleIntervalVal.shrinkIntervals(varPositionsL[index]->colorType->productSize());
                                auto intervalVec = leftTupleIntervalVal._intervals;

                                expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR[index], intervalVec, rightTupleInterval->tupleSize(), index + varPositionsL[index]->colorType->productSize());

                                Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(leftTupleIntervalVal._intervals, resizedRightIntervals);
                                Colored::intervalTuple_t newIntervalTupleR = getIntervalOverlap(rightTupleIntervalVal._intervals, intervalVec);

                                newIntervalTupleL.applyModifier(leftVarModifier, varPositionPair.second->colorType->getConstituentsSizes());
                                newIntervalTupleR.applyModifier(rightVarModifier, varPositionsR[index]->colorType->getConstituentsSizes());

                                *leftTupleInterval = newIntervalTupleL;
                                *rightTupleInterval = newIntervalTupleR;
                            }
                        } else {
                            auto rightColor = constantMapR[index];
                            auto leftTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            rightColor->getTupleId(&idVec);
                            int32_t leftVarModifier = getVarModifier(varModifierMapL[varPositionPair.second].back(), index);

                            std::vector<Colored::interval_t> intervals;
                            intervals.push_back(getIntervalFromIds(idVec, varPositionPair.second->colorType->size(), leftVarModifier));

                            expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR, varPositionPair.second, intervals, leftTupleInterval->tupleSize(), index + idVec.size());

                            Colored::intervalTuple_t newIntervalTupleL = getIntervalOverlap(leftTupleInterval->_intervals, intervals);
                            *leftTupleInterval = newIntervalTupleL;
                        }
                    }

                    for(auto varPositionPair : varPositionsR){
                        uint32_t index = varPositionPair.first;

                        if(constantMapL.count(index)){
                            auto leftColor = constantMapL[index];
                            auto rightTupleInterval = &varMap[varPositionPair.second];
                            std::vector<uint32_t> idVec;
                            leftColor->getTupleId(&idVec);
                            int32_t rightVarModifier = getVarModifier(varModifierMapR[varPositionPair.second].back(), index);

                            std::vector<Colored::interval_t> intervals;
                            intervals.push_back(getIntervalFromIds(idVec, varPositionPair.second->colorType->size(), rightVarModifier));

                            expandIntervalVec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL, varPositionsR[index], intervals, rightTupleInterval->tupleSize(), index + idVec.size());

                            Colored::intervalTuple_t newIntervalTupleR = getIntervalOverlap(rightTupleInterval->_intervals, intervals);
                            *rightTupleInterval = newIntervalTupleR;
                        }
                    }
                }
            }
        };        
    }
}


#endif /* GuardRestrictions_H */