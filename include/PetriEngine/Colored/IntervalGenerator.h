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
#ifndef INTERVALGENERATOR_H
#define INTERVALGENERATOR_H

#include "ColoredNetStructures.h"

namespace PetriEngine {
    struct IntervalGenerator {
        std::vector<Colored::interval_t> getIntervalsFromInterval(Colored::interval_t *interval, uint32_t varPosition, int32_t varModifier, std::vector<const Colored::ColorType*> *varColorTypes){
            std::vector<Colored::interval_t> varIntervals;
            Colored::interval_t firstVarInterval;
            varIntervals.push_back(firstVarInterval);
            for(uint32_t i = varPosition; i < varPosition + varColorTypes->size(); i++){
                auto ctSize = varColorTypes->operator[](i - varPosition)->size();
                int32_t lower_val = ctSize + (interval->operator[](i)._lower + varModifier);
                int32_t upper_val = ctSize + (interval->operator[](i)._upper + varModifier);
                uint32_t lower = lower_val % ctSize;
                uint32_t upper = upper_val % ctSize;

                if(lower > upper ){
                    if(lower == upper+1){
                        for (auto& varInterval : varIntervals){
                            varInterval.addRange(0, ctSize -1);
                        }
                    } else {
                        std::vector<Colored::interval_t> newIntervals;
                        for (auto& varInterval : varIntervals){
                            Colored::interval_t newVarInterval = varInterval;                                  
                            varInterval.addRange(0, upper);
                            newVarInterval.addRange(lower, ctSize -1);
                            newIntervals.push_back(newVarInterval);
                        }
                        varIntervals.insert(varIntervals.end(), newIntervals.begin(), newIntervals.end());
                    }                
                } else {
                    for (auto& varInterval : varIntervals){
                        varInterval.addRange(lower, upper);
                    }
                }            
            }
            return varIntervals;
        }

        void getArcVarIntervals(Colored::intervalTuple_t& varIntervals, std::unordered_map<uint32_t, int32_t> *modIndexMap, Colored::interval_t *interval, std::vector<const Colored::ColorType*> *varColorTypes){
            for(auto& posModPair : *modIndexMap){
                auto intervals = getIntervalsFromInterval(interval, posModPair.first, posModPair.second, varColorTypes);

                if(varIntervals._intervals.empty()){
                    for(auto& interval : intervals){
                        varIntervals.addInterval(std::move(interval));
                    }
                } else {
                    Colored::intervalTuple_t newVarIntervals; 
                    for(uint32_t i = 0; i < varIntervals.size(); i++){
                        auto varInterval = &varIntervals[i];
                        for(auto& interval : intervals){
                            auto overlap = varInterval->getOverlap(interval);
                            if(overlap.isSound()){ 
                                newVarIntervals.addInterval(std::move(overlap));
                                //break;
                            }                                                
                        }                                   
                    }                                                                                       
                    varIntervals = std::move(newVarIntervals);                                          
                }     
            } 
        }

        void populateLocalMap(Colored::ArcIntervals *arcIntervals, 
                            Colored::VariableIntervalMap &varMap,
                            Colored::VariableIntervalMap &localVarMap,
                            Colored::interval_t* interval, bool& allVarsAssigned,  uint32_t tuplePos){
            for(auto& pair : arcIntervals->_varIndexModMap){                     
                Colored::intervalTuple_t varIntervals; 
                std::vector<const Colored::ColorType*> varColorTypes;
                pair.first->colorType->getColortypes(varColorTypes);
                
                getArcVarIntervals(varIntervals, &pair.second[tuplePos], interval, &varColorTypes);                          
                
                if (arcIntervals->_intervalTupleVec.size() > 1 && pair.second[tuplePos].empty()) {
                    //The variable is not on this side of the add expression, so we add a full interval to compare against for the other side
                    varIntervals.addInterval(pair.first->colorType->getFullInterval());
                }

                if(varMap.count(pair.first) == 0){
                    localVarMap[pair.first] = std::move(varIntervals);                                  
                } else {                                    
                    for(auto& varInterval : varIntervals._intervals){
                        for(auto& interval : varMap[pair.first]._intervals){
                            auto overlapInterval = varInterval.getOverlap(interval);

                            if(overlapInterval.isSound()){
                                localVarMap[pair.first].addInterval(overlapInterval);
                            }
                        }
                    }                                    
                }

                if(localVarMap[pair.first]._intervals.empty()){
                    allVarsAssigned = false;
                }                                
            }         
        }

        void fillVarMaps(std::vector<Colored::VariableIntervalMap> &variableMaps,
                                            Colored::ArcIntervals *arcIntervals,
                                            uint32_t *intervalTupleSize,
                                            uint32_t *tuplePos)
        {
            for(uint32_t i = 0; i < *intervalTupleSize; i++){
                Colored::VariableIntervalMap localVarMap;
                bool validInterval = true;
                auto interval = &arcIntervals->_intervalTupleVec[*tuplePos]._intervals[i];
            
                for(auto pair : arcIntervals->_varIndexModMap){
                    Colored::intervalTuple_t varIntervals;
                    std::vector<const Colored::ColorType*> varColorTypes;
                    pair.first->colorType->getColortypes(varColorTypes);
                    getArcVarIntervals(varIntervals, &pair.second[*tuplePos], interval, &varColorTypes); 

                    if(arcIntervals->_intervalTupleVec.size() > 1 && pair.second[*tuplePos].empty()){
                        //The variable is not on this side of the add expression, so we add a full interval to compare against for the other side
                        varIntervals.addInterval(pair.first->colorType->getFullInterval());
                    } else if(varIntervals.size() < 1){
                        //If any varinterval ends up empty then we were unable to use this arc interval
                        validInterval = false;
                        break;
                    }
                    localVarMap[pair.first] = varIntervals;
                }

                if(validInterval){
                    variableMaps.push_back(localVarMap);
                }                          
            }  
        }

        bool getVarIntervals(std::vector<Colored::VariableIntervalMap>& variableMaps, std::unordered_map<uint32_t, Colored::ArcIntervals> placeArcIntervals){
            for(auto& placeArcInterval : placeArcIntervals){            
                for(uint32_t j = 0; j < placeArcInterval.second._intervalTupleVec.size(); j++){
                    uint32_t intervalTupleSize = placeArcInterval.second._intervalTupleVec[j].size();
                    //If we have not found intervals for any place yet, we fill the intervals from this place
                    //Else we restrict the intervals we already found to only keep those that can also be matched in this place
                    if(variableMaps.empty()){                    
                        fillVarMaps(variableMaps, &placeArcInterval.second, &intervalTupleSize, &j);                                            
                    } else {
                        std::vector<Colored::VariableIntervalMap> newVarMapVec;
                        
                        for(auto& varMap : variableMaps){          
                            for(uint32_t i = 0; i < intervalTupleSize; i++){
                                Colored::VariableIntervalMap localVarMap;
                                bool allVarsAssigned = true;
                                auto interval = &placeArcInterval.second._intervalTupleVec[j]._intervals[i];
                                
                                populateLocalMap(&placeArcInterval.second, varMap, localVarMap, interval, allVarsAssigned, j);                       
                                
                                for(auto& varTuplePair : varMap){
                                    if(localVarMap.count(varTuplePair.first) == 0){
                                        localVarMap[varTuplePair.first] = std::move(varTuplePair.second);
                                    }
                                }

                                if(allVarsAssigned){
                                    newVarMapVec.push_back(std::move(localVarMap));
                                }
                                                               
                            }                                                                                    
                        }               
                        variableMaps = std::move(newVarMapVec);                    
                    }
                    //If we did not find any intervals for an arc, then the transition cannot be activated
                    if(variableMaps.empty()){
                        return false;
                    }                          
                }
            }
            return true;
        }
    };    
}

#endif /* INTERVALGENERATOR_H */