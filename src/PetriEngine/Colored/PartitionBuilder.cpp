#include "PetriEngine/Colored/PartitionBuilder.h"
#include <numeric>



namespace PetriEngine {
    namespace Colored {

        PartitionBuilder::PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap) 
        : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap), _placePreTransitionMap(placePreTransitionMap) {

            //Instantiate partitions
            for(uint32_t i = 0; i < _places->size(); i++){
                auto place = _places->operator[](i);
                EquivalenceClass fullClass;
                interval_t fullInterval = place.type->getFullInterval();
                fullClass._colorIntervals.addInterval(fullInterval); 
                fullClass._colorType = place.type;
                _partition[i]._equivalenceClasses.push_back(fullClass);
                _placeQueue.push_back(i);
                _inQueue[i] = true;
            }
        }

        void PartitionBuilder::printPartion() {
            for(auto equivalenceVec : _partition){
                std::cout << "Partition for place " << _places->operator[](equivalenceVec.first).name << std::endl;
                for (auto equivalenceClass : equivalenceVec.second._equivalenceClasses){
                    std::cout << equivalenceClass.toString() << std::endl;
                    
                }
                std::cout << "Diagonal " << equivalenceVec.second.diagonal << std::endl << std::endl;;
            }
        }

       

        
        void PartitionBuilder::partitionNet(){
            while(!_placeQueue.empty()){
                auto placeId = _placeQueue.back();
                _placeQueue.pop_back();
                _inQueue[placeId] = false;

                auto place = _places->operator[](placeId);

                for(uint32_t transitionId : _placePreTransitionMap->operator[](placeId)){
                    std::cout << "For transition " << _transitions->operator[](transitionId).name << " and place " << _places->operator[](placeId).name << std::endl;
                    printPartion();                    
                    handleTransition(transitionId, placeId);
                    std::cout << "---------------------------------------------------" << std::endl;
                }               
            }
        }

        void PartitionBuilder::handleTransition(uint32_t transitionId, uint32_t postPlaceId){
            std::unordered_map<const PetriEngine::Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap;
            std::set<const Colored::Variable*> diagonalVars;
            std::unordered_map<uint32_t, const PetriEngine::Colored::Variable *> varPositionMap;
            std::set<const PetriEngine::Colored::Variable *> postArcVars;
            std::set<const PetriEngine::Colored::Variable *>guardVars;

            auto transition = _transitions->operator[](transitionId);
            Arc postArc;
            bool arcFound = false;
            for(auto& outArc : transition.output_arcs){
                if(outArc.place == postPlaceId){
                    postArc = outArc;
                    arcFound = true;
                    break;
                }
            }

            if(!arcFound){
                return;
            }           
            
            postArc.expr->getVariables(postArcVars, varPositionMap, varModifierMap);

            if(transition.guard != nullptr){
                transition.guard->getVariables(guardVars);
            }

            auto placePartition = _partition[postPlaceId]._equivalenceClasses;
            
            for(auto eqClass : placePartition){
                auto varMaps = prepareVariables(varModifierMap, &eqClass, &postArc, postPlaceId);

                for(auto& varMap : varMaps){
                    for(auto var : guardVars){
                        if(varMap.count(var) == 0){
                            varMap[var].addInterval(var->colorType->getFullInterval());
                        }                            
                    }
                }

                if(_transitions->operator[](transitionId).guard != nullptr){
                    _transitions->operator[](transitionId).guard->restrictVars(varMaps, diagonalVars);
                }

                for(auto inArc : transition.input_arcs){
                    if(_partition[inArc.place].diagonal){
                        continue;
                    }

                    std::set<const PetriEngine::Colored::Variable *> preArcVars;
                    inArc.expr->getVariables(preArcVars); 
                    for(auto preVar : preArcVars){
                        if(diagonalVars.count(preVar)){
                            //should only happen if the variable is not in a tuple 
                            _partition[inArc.place].diagonal = true;
                            break;
                        }
                    }

                    auto outIntervals = inArc.expr->getOutputIntervals(varMaps);
                    outIntervals.simplify();
                    EquivalenceVec newEqVec;
                    EquivalenceClass newEqClass;
                    newEqClass._colorIntervals = outIntervals;
                    newEqClass._colorType = _partition[inArc.place]._equivalenceClasses.back()._colorType;

                    newEqVec._equivalenceClasses.push_back(newEqClass);

                    if(splitPartition(newEqVec, inArc.place) && !_inQueue[inArc.place]){
                        _placeQueue.push_back(inArc.place);
                        _inQueue[inArc.place] = true;
                    }
                }
            }

            std::set<const PetriEngine::Colored::Variable *> preArcVars;
            for(auto inArc : transition.input_arcs){
                inArc.expr->getVariables(preArcVars);
                for(auto postVar : postArcVars){
                    if(preArcVars.count(postVar)){
                        if(diagonalVars.count(postVar)){
                            _partition[inArc.place].diagonal = true;
                        }
                    }
                }
            }
            
        }


        bool PartitionBuilder::splitPartition(PetriEngine::Colored::EquivalenceVec equivalenceVec, uint32_t placeId){
            bool split = false;
            if(_partition.count(placeId) == 0){
                _partition[placeId] = equivalenceVec;
            } else {
                EquivalenceClass intersection;
                uint32_t ecPos1 = 0, ecPos2 = 0;
                while(findOverlap(equivalenceVec, _partition[placeId],ecPos1, ecPos2, intersection)) {
                    auto ec1 = equivalenceVec._equivalenceClasses[ecPos1];
                    auto ec2 = _partition[placeId]._equivalenceClasses[ecPos2];
                    auto rightSubtractEc = ec1.subtract(ec2);
                    auto leftSubtractEc = ec2.subtract(ec1);
                    std::cout << "comparing " << ec2.toString() << " to " << ec1.toString() << std::endl;

                    std::cout << "Intersection: " << intersection.toString() << std::endl;
                    std::cout << "Left: " << leftSubtractEc.toString() << std::endl;
                    std::cout << "Right: " << rightSubtractEc.toString() << std::endl;

                    equivalenceVec._equivalenceClasses.erase(equivalenceVec._equivalenceClasses.begin() + ecPos1);
                    _partition[placeId]._equivalenceClasses.erase(_partition[placeId]._equivalenceClasses.begin() + ecPos2);

                    if(!intersection.isEmpty()){
                        _partition[placeId]._equivalenceClasses.push_back(intersection);
                        intersection._colorIntervals._intervals.clear();
                    }
                    if(!leftSubtractEc.isEmpty()){
                        _partition[placeId]._equivalenceClasses.push_back(leftSubtractEc);
                        split = true;
                    }
                    if(!rightSubtractEc.isEmpty()){
                        equivalenceVec._equivalenceClasses.push_back(rightSubtractEc);
                    }                                      
                }
            }
            return split;
        }

        bool PartitionBuilder::findOverlap(EquivalenceVec equivalenceVec1, EquivalenceVec equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection){
            for(uint32_t i = 0; i < equivalenceVec1._equivalenceClasses.size(); i++){
                for(uint32_t j = 0; j < equivalenceVec2._equivalenceClasses.size(); j++){
                    auto ec = equivalenceVec1._equivalenceClasses[i];
                    auto ec2 = equivalenceVec2._equivalenceClasses[j];
                    
                    auto intersectingEc = ec.intersect(ec2);                    
                    if(!intersectingEc.isEmpty()){
                        overlap1 = i;
                        overlap2 = j;
                        intersection = intersectingEc;
                        return true;
                    }
                }
            }
            return false;
        }

        std::vector<std::unordered_map<const Variable *, intervalTuple_t>> 
        PartitionBuilder::prepareVariables(
                    std::unordered_map<const Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap, 
                    EquivalenceClass *eqClass , Arc *postArc, uint32_t placeId){
            std::vector<std::unordered_map<const Variable *, intervalTuple_t>> varMaps;
            std::unordered_map<const Variable *, intervalTuple_t> varMap;
            varMaps.push_back(varMap);
            std::unordered_map<uint32_t, ArcIntervals> placeArcIntervals;
            ColorFixpoint postPlaceFixpoint;
            postPlaceFixpoint.constraints = eqClass->_colorIntervals; 
            ArcIntervals newArcInterval(&postPlaceFixpoint, varModifierMap);
            uint32_t index = 0;

            postArc->expr->getArcIntervals(newArcInterval, postPlaceFixpoint, &index, 0);
            placeArcIntervals[placeId] = newArcInterval;
            intervalGenerator.getVarIntervals(varMaps, placeArcIntervals);

            return varMaps;                
        }
    }
}