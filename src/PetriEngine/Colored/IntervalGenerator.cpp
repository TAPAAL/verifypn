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

#include "PetriEngine/Colored/IntervalGenerator.h"

namespace PetriEngine {
    namespace Colored {

        std::vector<interval_t> IntervalGenerator::getIntervalsFromInterval(const interval_t &interval, uint32_t varPosition, int32_t varModifier, const std::vector<const ColorType*> &varColorTypes) {
            std::vector<interval_t> varIntervals;
            varIntervals.emplace_back();
            for(uint32_t i = varPosition; i < varPosition + varColorTypes.size(); i++){
                auto ctSize = varColorTypes[i - varPosition]->size();
                int32_t lower_val = ctSize + (interval[i]._lower + varModifier);
                int32_t upper_val = ctSize + (interval[i]._upper + varModifier);
                uint32_t lower = lower_val % ctSize;
                uint32_t upper = upper_val % ctSize;

                if(lower > upper ){
                    if(lower == upper+1){
                        for (auto& varInterval : varIntervals){
                            varInterval.addRange(0, ctSize -1);
                        }
                    } else {
                        std::vector<interval_t> newIntervals;
                        for (auto& varInterval : varIntervals){
                            interval_t newVarInterval = varInterval;
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

        void IntervalGenerator::getArcVarIntervals(interval_vector_t& varIntervals, const std::unordered_map<uint32_t, int32_t> &modIndexMap, const interval_t &interval, const std::vector<const ColorType*> &varColorTypes) {
            for(auto& posModPair : modIndexMap){
                const auto &intervals = getIntervalsFromInterval(interval, posModPair.first, posModPair.second, varColorTypes);

                if(varIntervals.empty()){
                    for(auto& interval : intervals){
                        varIntervals.addInterval(std::move(interval));
                    }
                } else {
                    interval_vector_t newVarIntervals;
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

        void IntervalGenerator::populateLocalMap(const ArcIntervals &arcIntervals,
                            const VariableIntervalMap &varMap,
                            VariableIntervalMap &localVarMap,
                            const interval_t &interval, bool& allVarsAssigned,  uint32_t tuplePos) {
            for(const auto& pair : arcIntervals._varIndexModMap){
                interval_vector_t varIntervals;
                std::vector<const ColorType*> varColorTypes;
                pair.first->colorType->getColortypes(varColorTypes);

                getArcVarIntervals(varIntervals, pair.second[tuplePos], interval, varColorTypes);

                if (arcIntervals._intervalTupleVec.size() > 1 && pair.second[tuplePos].empty()) {
                    //The variable is not on this side of the add expression, so we add a full interval to compare against for the other side
                    varIntervals.addInterval(pair.first->colorType->getFullInterval());
                }

                if(varMap.count(pair.first) == 0){
                    localVarMap[pair.first] = std::move(varIntervals);
                } else {
                    for(const auto& varInterval : varIntervals){
                        for(const auto& interval : varMap.find(pair.first)->second){
                            auto overlapInterval = varInterval.getOverlap(interval);

                            if(overlapInterval.isSound()){
                                localVarMap[pair.first].addInterval(overlapInterval);
                            }
                        }
                    }
                }

                if(localVarMap[pair.first].empty()){
                    allVarsAssigned = false;
                }
            }
        }

        void IntervalGenerator::fillVarMaps(std::vector<VariableIntervalMap> &variableMaps,
                                            const ArcIntervals &arcIntervals,
                                            const uint32_t &intervalTupleSize,
                                            const uint32_t &tuplePos)
        {
            for(uint32_t i = 0; i < intervalTupleSize; i++){
                VariableIntervalMap localVarMap;
                bool validInterval = true;
                const auto &interval = arcIntervals._intervalTupleVec[tuplePos][i];

                for(const auto &pair : arcIntervals._varIndexModMap){
                    interval_vector_t varIntervals;
                    std::vector<const ColorType*> varColorTypes;
                    pair.first->colorType->getColortypes(varColorTypes);
                    
                    assert(pair.second.size() > tuplePos);
                    getArcVarIntervals(varIntervals, pair.second[tuplePos], interval, varColorTypes);

                    if(arcIntervals._intervalTupleVec.size() > 1 && pair.second[tuplePos].empty()){
                        //The variable is not on this side of the add expression, so we add a full interval to compare against for the other side
                        varIntervals.addInterval(pair.first->colorType->getFullInterval());
                    } else if(varIntervals.size() < 1){
                        //If any varinterval ends up empty then we were unable to use this arc interval
                        validInterval = false;
                        break;
                    }
                    localVarMap[pair.first] = varIntervals;
                }

                //Only add valid intervals. Ensure one empty map is always added if there are intervals
                if(validInterval && (!localVarMap.empty() || variableMaps.empty())){
                    variableMaps.push_back(std::move(localVarMap));
                }
            }
        }

        bool IntervalGenerator::getVarIntervals(std::vector<VariableIntervalMap>& variableMaps, const std::unordered_map<uint32_t, ArcIntervals> &placeArcIntervals) {
            for(auto& placeArcInterval : placeArcIntervals){
                for(uint32_t j = 0; j < placeArcInterval.second._intervalTupleVec.size(); j++){
                    uint32_t intervalTupleSize = placeArcInterval.second._intervalTupleVec[j].size();
                    //If we have not found intervals for any place yet, we fill the intervals from this place
                    //Else we restrict the intervals we already found to only keep those that can also be matched in this place
                    if(variableMaps.empty()){
                        fillVarMaps(variableMaps, placeArcInterval.second, intervalTupleSize, j);
                    } else {
                        std::vector<VariableIntervalMap> newVarMapVec;

                        for(const auto& varMap : variableMaps){
                            for(uint32_t i = 0; i < intervalTupleSize; i++){
                                VariableIntervalMap localVarMap;
                                bool allVarsAssigned = true;
                                auto interval = placeArcInterval.second._intervalTupleVec[j][i];

                                populateLocalMap(placeArcInterval.second, varMap, localVarMap, interval, allVarsAssigned, j);

                                for(const auto& varTuplePair : varMap){
                                    if(localVarMap.count(varTuplePair.first) == 0){
                                        localVarMap[varTuplePair.first] = std::move(varTuplePair.second);
                                    }
                                }

                                if(allVarsAssigned && (!localVarMap.empty() || newVarMapVec.empty())){
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
    }
}