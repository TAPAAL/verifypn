#include "PetriEngine/Colored/PartitionBuilder.h"

namespace PetriEngine {
    namespace Colored {

        PartitionBuilder::PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap) 
        : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap) {
            
        }

        void PartitionBuilder::initPartition(){

            for(uint32_t i = 0; i < _places->size(); i++){
                auto place = _places->operator[](i);
                Colored::Arc *inputArc;
                for(auto transitionId : _placePostTransitionMap->operator[](i)){
                    for(auto arc : _transitions->operator[](transitionId).input_arcs){
                        if(arc.place == i){
                            inputArc = &arc;
                            break;
                        }
                    } 
                    std::set<const Variable *> arcVariables;
                    std::unordered_map<uint32_t, std::vector<const Color*>> arcConstants;
                    uint32_t index = 0;
                    EquivalenceVec placeECVec;

                    inputArc->expr->getVariables(arcVariables);
                    inputArc->expr->getConstants(arcConstants, index);

                    if(arcVariables.empty()){
                        for(uint32_t j = 0; j < arcConstants[0].size(); j++){
                            EquivalenceClass ec;
                            ec._colorType = place.type;
                            Colored::intervalTuple_t colorIntervals;
                            Colored::interval_t colorInterval;
                            for(auto positionColors : arcConstants){
                                auto colorId = positionColors.second[j]->getId();
                                colorInterval.addRange(colorId, colorId);
                            }                            
                            ec._colorIntervals.addInterval(std::move(colorInterval));
                            placeECVec._equivalenceClasses.push_back(ec);
                        }                       
                    } else {
                        std::vector<std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t>> varMaps;
                        std::unordered_map<const Colored::Variable *, Colored::intervalTuple_t> varMap;
                        for(auto var : arcVariables){
                            varMap[var].addInterval(var->colorType->getFullInterval());
                        }
                        varMaps.push_back(varMap);
                        _transitions->operator[](transitionId).guard->restrictVars(varMaps, true, placeECVec.diagonal);
                        if(!arcConstants.empty()){
                            //handle constants and handle tuples
                            //what happens for (x, 2) + (y,z)
                            //or x + 4
                        }

                    }
                }
            }
        }
    }
}