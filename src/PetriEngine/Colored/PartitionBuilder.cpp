#include "PetriEngine/Colored/PartitionBuilder.h"
#include <numeric>
#include <chrono>



namespace PetriEngine {
    namespace Colored {

        PartitionBuilder::PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap) 
        : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap), _placePreTransitionMap(placePreTransitionMap) {

            //Instantiate partitions
            for(uint32_t i = 0; i < _places->size(); i++){
                const PetriEngine::Colored::Place& place = _places->operator[](i);
                interval_t fullInterval = place.type->getFullInterval();
                EquivalenceClass fullClass = EquivalenceClass(place.type);
                fullClass._colorIntervals.addInterval(fullInterval); 
                _partition[i]._equivalenceClasses.push_back(fullClass);
                for(uint32_t j = 0; j < place.type->productSize(); j++){
                    _partition[i].diagonalTuplePositions.push_back(false);
                }
                _placeQueue.push_back(i);
                _inQueue[i] = true;
            }
        }

        PartitionBuilder::PartitionBuilder(std::vector<Transition> *transitions, std::vector<Place> *places, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePostTransitionMap, std::unordered_map<uint32_t,std::vector<uint32_t>> *placePreTransitionMap, std::vector<Colored::ColorFixpoint> *placeColorFixpoints) 
        : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap), _placePreTransitionMap(placePreTransitionMap) {

            //Instantiate partitions
            for(uint32_t i = 0; i < _places->size(); i++){
                const PetriEngine::Colored::Place& place = _places->operator[](i);
                EquivalenceClass fullClass = EquivalenceClass(place.type);
                fullClass._colorIntervals = placeColorFixpoints->operator[](i).constraints;
                _partition[i]._equivalenceClasses.push_back(fullClass);
                for(uint32_t j = 0; j < place.type->productSize(); j++){
                    _partition[i].diagonalTuplePositions.push_back(false);
                }
                _placeQueue.push_back(i);
                _inQueue[i] = true;
            }
        }

        void PartitionBuilder::printPartion() {
            for(auto equivalenceVec : _partition){
                std::cout << "Partition for place " << _places->operator[](equivalenceVec.first).name << std::endl;
                std::cout << "Diag variables: (";
                for(auto daigPos : equivalenceVec.second.diagonalTuplePositions){
                    std::cout << daigPos << ",";
                }
                std::cout << ")" << std::endl;
                for (auto equivalenceClass : equivalenceVec.second._equivalenceClasses){
                    std::cout << equivalenceClass.toString() << std::endl;
                    
                }
                std::cout << "Diagonal " << equivalenceVec.second.diagonal << std::endl << std::endl;;
            }
        }
        
        void PartitionBuilder::partitionNet(){
            handleLeafTransitions();            
            
            while(!_placeQueue.empty()){
                auto placeId = _placeQueue.back();
                _placeQueue.pop_back();
                _inQueue[placeId] = false;

                auto place = _places->operator[](placeId);

                for(uint32_t transitionId : _placePreTransitionMap->operator[](placeId)){
                    std::cout << "For transition " << _transitions->operator[](transitionId).name << " and place " << _places->operator[](placeId).name << std::endl;
                    printPartion(); 
                                      
                    handleTransition(transitionId, placeId);
                    
                    //std::cout << "---------------------------------------------------" << std::endl;
                }               
            }          
        }

        void PartitionBuilder::assignColorMap(std::unordered_map<uint32_t, EquivalenceVec> &partition){
            for(auto& eqVec : partition){
                if(eqVec.second.diagonal){
                    continue;
                }
                ColorType *colorType = _places->operator[](eqVec.first).type;
                for(uint32_t i = 0; i < colorType->size(); i++){ 
                    const Color *color = &colorType->operator[](i);
                    uint32_t colorAdded = 0;                    
                    for(auto& eqClass : eqVec.second._equivalenceClasses){   
                        std::vector<uint32_t> colorIds;
                        color->getTupleId(&colorIds);
                        if(eqClass.containsColor(colorIds, eqVec.second.diagonalTuplePositions)){
                            eqVec.second.colorEQClassMap[color] = &eqClass;
                            colorAdded++;
                        }
                    }                    
                }               
            }
        }

        void PartitionBuilder::handleTransition(uint32_t transitionId, uint32_t postPlaceId){
            const PetriEngine::Colored::Transition &transition = _transitions->operator[](transitionId);
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
                        
            handleTransition(transition, postPlaceId, &postArc);
        }

        void PartitionBuilder::handleTransition(const Transition &transition, const uint32_t postPlaceId, const Arc *postArc) {
            std::unordered_map<const PetriEngine::Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varModifierMap;
            std::unordered_map<uint32_t, const PetriEngine::Colored::Variable *> varPositionMap;
            std::set<const PetriEngine::Colored::Variable *> postArcVars;
            std::set<const PetriEngine::Colored::Variable *>guardVars;
            std::set<const Colored::Variable*> diagonalVars;
            
            postArc->expr->getVariables(postArcVars, varPositionMap, varModifierMap, true);

            for(auto varModMap : varModifierMap){
                if(varModMap.second.size() > 1){
                    uint32_t actualSize = 0;
                    std::vector<uint32_t> positions;
                    for(auto map : varModMap.second){
                        if(!map.empty()){
                            for(auto position : map){
                                positions.push_back(position.first);
                            }                            
                            actualSize++;
                        }
                    }
                    if(actualSize > 1) {
                        if(_partition[postPlaceId]._equivalenceClasses.back()._colorType->productSize() == 1){
                            _partition[postPlaceId].diagonal = true;
                        } else {
                            diagonalVars.insert(varModMap.first);
                            for(auto pos : positions){
                                _partition[postPlaceId].diagonalTuplePositions[pos] = true;
                            }
                        }
                    } 
                }
            }

            if(transition.guard != nullptr){
                transition.guard->getVariables(guardVars);
            }

            auto placePartition = _partition[postPlaceId]._equivalenceClasses;
            
            for(auto eqClass : placePartition){
                auto varMaps = prepareVariables(varModifierMap, &eqClass, postArc, postPlaceId);

                for(auto& varMap : varMaps){
                    for(auto var : guardVars){
                        if(varMap.count(var) == 0){
                            varMap[var].addInterval(var->colorType->getFullInterval());
                        }                            
                    }
                }

                if(transition.guard != nullptr){
                    transition.guard->restrictVars(varMaps, diagonalVars);
                }

                std::unordered_map<uint32_t, std::unordered_map<uint32_t, const PetriEngine::Colored::Variable *>> placeVariableMap;
                for(auto inArc : transition.input_arcs){
                    //Hack to avoid considering dot places and dealing with retrieving the correct dot pointer
                    if(_places->operator[](inArc.place).type->getName() == "Dot" || _places->operator[](inArc.place).type->getName() == "dot"){
                        _partition[inArc.place].diagonal = true;
                    }

                    if(_partition[inArc.place].diagonal){
                        continue;
                    }
                    
                    std::unordered_map<const PetriEngine::Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> preVarModifierMap;
                    std::unordered_map<uint32_t, const PetriEngine::Colored::Variable *> preVarPositionMap;
                    std::set<const PetriEngine::Colored::Variable *> preArcVars;
                    inArc.expr->getVariables(preArcVars, preVarPositionMap, preVarModifierMap, true);
                    for(auto placeVariables : placeVariableMap){
                        for(auto variable : preVarPositionMap){
                            for(auto varPosition : placeVariables.second){
                                if(varPosition.second == variable.second){
                                    if(_partition[inArc.place]._equivalenceClasses.back()._colorType->productSize() == 1){
                                        _partition[inArc.place].diagonal = true;
                                        addToQueue(inArc.place);
                                    } else {
                                        diagonalVars.insert(variable.second);
                                        
                                        if(!_partition[inArc.place].diagonalTuplePositions[variable.first]){
                                            addToQueue(placeVariables.first);
                                            _partition[inArc.place].diagonalTuplePositions[variable.first] = true;
                                        }
                                    } 

                                    if(_partition[placeVariables.first]._equivalenceClasses.back()._colorType->productSize() == 1){
                                        _partition[placeVariables.first].diagonal = true;
                                        addToQueue(placeVariables.first);
                                    } else {
                                        if(!_partition[placeVariables.first].diagonalTuplePositions[varPosition.first]){
                                            addToQueue(placeVariables.first);
                                            _partition[placeVariables.first].diagonalTuplePositions[varPosition.first] = true;
                                        }                                        
                                    } 
                                    
                                    break;                                
                                }
                            }
                            if(_partition[inArc.place].diagonal){
                                break;
                            }                            
                        }
                        if(_partition[inArc.place].diagonal){
                            break;
                        }
                    }
                    placeVariableMap[inArc.place] = preVarPositionMap;

                    if(_partition[inArc.place].diagonal){
                        addToQueue(inArc.place);
                        continue;
                    }

                    
                    for(auto preVar : preVarPositionMap){
                        for(auto postVar : varPositionMap){
                            if(preVar.second == postVar.second){
                                if(_partition[postPlaceId].diagonal || _partition[postPlaceId].diagonalTuplePositions[postVar.first]){
                                    if(_partition[inArc.place]._equivalenceClasses.back()._colorType->productSize() == 1){
                                        _partition[inArc.place].diagonal = true;
                                    } else {
                                        diagonalVars.insert(preVar.second);
                                        if(!_partition[inArc.place].diagonalTuplePositions[preVar.first]){
                                            addToQueue(inArc.place);
                                            _partition[inArc.place].diagonalTuplePositions[preVar.first] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    

                    if(_partition[inArc.place].diagonal){
                        addToQueue(inArc.place);
                        continue;
                    }

                    for(auto varModMap : preVarModifierMap){
                        if(varModMap.second.size() > 1){
                            uint32_t actualSize = 0;
                            std::vector<uint32_t> positions;
                            for(auto map : varModMap.second){
                                if(!map.empty()){
                                    for(auto position : map){
                                        positions.push_back(position.first);
                                    }
                                    actualSize++;
                                }
                            }
                            if(actualSize > 1) {
                                if(_partition[inArc.place]._equivalenceClasses.back()._colorType->productSize() == 1){
                                    _partition[inArc.place].diagonal = true;
                                } else {
                                    diagonalVars.insert(varModMap.first);
                                    for(auto pos : positions){
                                        if(!_partition[inArc.place].diagonalTuplePositions[pos]){
                                            addToQueue(inArc.place);
                                            _partition[inArc.place].diagonalTuplePositions[pos] = true;
                                        }
                                    }
                                }
                                
                            }                            
                        }
                    }

                    bool allPositionsDiagonal = true;
                    for(auto diag : _partition[inArc.place].diagonalTuplePositions){
                        if(!diag){
                            allPositionsDiagonal = false;
                            break;
                        }
                    }

                    if(allPositionsDiagonal){
                        _partition[inArc.place].diagonal = true;
                        addToQueue(inArc.place);
                        continue;
                    }


                    for(auto preVar : preVarPositionMap){
                        if(diagonalVars.count(preVar.second)){
                            if(_partition[inArc.place]._equivalenceClasses.back()._colorType->productSize() == 1){
                                _partition[inArc.place].diagonal = true;
                                addToQueue(inArc.place);
                                break;
                            } else {
                                if(!_partition[inArc.place].diagonalTuplePositions[preVar.first]){
                                    addToQueue(inArc.place);
                                    _partition[inArc.place].diagonalTuplePositions[preVar.first] = true;
                                }
                            }                           
                        }
                    }

                    auto outIntervals = inArc.expr->getOutputIntervals(varMaps);
                    outIntervals.simplify();
                    EquivalenceVec newEqVec;
                    EquivalenceClass newEqClass = EquivalenceClass(_partition[inArc.place]._equivalenceClasses.back()._colorType, outIntervals);
                    newEqVec._equivalenceClasses.push_back(newEqClass);
                    newEqVec.diagonalTuplePositions = _partition[inArc.place].diagonalTuplePositions;

                    allPositionsDiagonal = true;
                    for(auto diag : _partition[inArc.place].diagonalTuplePositions){
                        if(!diag){
                            allPositionsDiagonal = false;
                            break;
                        }
                    }

                    if(allPositionsDiagonal || _partition[inArc.place]._equivalenceClasses.size() >= _partition[inArc.place]._equivalenceClasses.back()._colorType->size(&_partition[inArc.place].diagonalTuplePositions)){
                        _partition[inArc.place].diagonal = true;
                    }

                    if((_partition[inArc.place].diagonal || splitPartition(std::move(newEqVec), inArc.place))){
                        addToQueue(inArc.place);
                    }
                }
            }
        }

        void PartitionBuilder::addToQueue(uint32_t placeId){
            if(!_inQueue[placeId]){
                _placeQueue.push_back(placeId);
                _inQueue[placeId] = true;
            }
        }


        bool PartitionBuilder::splitPartition(PetriEngine::Colored::EquivalenceVec equivalenceVec, uint32_t placeId){
            bool split = false;
            if(_partition.count(placeId) == 0){
                _partition[placeId] = equivalenceVec;
            } else {
                EquivalenceClass intersection = EquivalenceClass();
                uint32_t ecPos1 = 0, ecPos2 = 0;
                while(findOverlap(equivalenceVec, _partition[placeId],ecPos1, ecPos2, intersection)) {
                    auto ec1 = equivalenceVec._equivalenceClasses[ecPos1];
                    auto ec2 = _partition[placeId]._equivalenceClasses[ecPos2];
                    auto rightSubtractEc = ec1.subtract(ec2, equivalenceVec.diagonalTuplePositions, false);
                    auto leftSubtractEc = ec2.subtract(ec1, _partition[placeId].diagonalTuplePositions, false);
                    //if((_places->operator[](placeId).name == "NB_ATTENTE_A") || (_places->operator[](placeId).name == "COMPTEUR") || (_places->operator[](placeId).name == "NB_ATTENTE_B")) {
                        // std::cout << _places->operator[](placeId).name << std::endl;
                        // std::cout << "comparing " << ec2.toString() << " to " << ec1.toString() << std::endl;

                        // std::cout << "Intersection: " << intersection.toString() << std::endl;
                        // std::cout << "Left: " << leftSubtractEc.toString() << std::endl;
                        // std::cout << "Right: " << rightSubtractEc.toString() << std::endl;
                        //ec2.subtract(ec1, true);
                    //}
                    

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
                    // std::cout << "Partition is now: " << std::endl;
                    // for(auto eqClass : _partition[placeId]._equivalenceClasses){
                    //     std::cout << eqClass.toString() << std::endl;
                    // }                                       
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
                    EquivalenceClass *eqClass , const Arc *arc, uint32_t placeId){
            std::vector<std::unordered_map<const Variable *, intervalTuple_t>> varMaps;
            std::unordered_map<const Variable *, intervalTuple_t> varMap;
            varMaps.push_back(varMap);
            std::unordered_map<uint32_t, ArcIntervals> placeArcIntervals;
            ColorFixpoint postPlaceFixpoint;
            postPlaceFixpoint.constraints = eqClass->_colorIntervals; 
            ArcIntervals newArcInterval(&postPlaceFixpoint, varModifierMap);
            uint32_t index = 0;

            arc->expr->getArcIntervals(newArcInterval, postPlaceFixpoint, &index, 0);
            placeArcIntervals[placeId] = newArcInterval;
            intervalGenerator.getVarIntervals(varMaps, placeArcIntervals);

            return varMaps;                
        }

        void PartitionBuilder::handleLeafTransitions(){
            for(uint32_t i = 0; i < _transitions->size(); i++){
                const Transition &transition = _transitions->operator[](i);
                if(transition.output_arcs.empty() && !transition.input_arcs.empty()){
                    handleTransition(transition, transition.input_arcs.back().place, &transition.input_arcs.back());
                }
            }
        }
    }    
}