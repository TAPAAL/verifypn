#include "PetriEngine/Colored/PartitionBuilder.h"
#include <numeric>
#include <chrono>



namespace PetriEngine {
    namespace Colored {

        PartitionBuilder::PartitionBuilder(const std::vector<Transition> &transitions, const std::vector<Place> &places, const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePostTransitionMap, const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePreTransitionMap) 
        : PartitionBuilder(transitions, places, placePostTransitionMap, placePreTransitionMap, nullptr){
        }

        PartitionBuilder::PartitionBuilder(const std::vector<Transition> &transitions, const std::vector<Place> &places, const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePostTransitionMap, const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePreTransitionMap, const std::vector<Colored::ColorFixpoint> *placeColorFixpoints) 
        : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap), _placePreTransitionMap(placePreTransitionMap) {

            //Instantiate partitions
            for(uint32_t i = 0; i < _places.size(); i++){
                const PetriEngine::Colored::Place& place = _places[i];
                EquivalenceClass fullClass = EquivalenceClass(place.type);
                if(placeColorFixpoints != nullptr){
                    fullClass._colorIntervals = placeColorFixpoints->operator[](i).constraints;
                } else {
                    interval_t fullInterval = place.type->getFullInterval();
                    fullClass._colorIntervals.addInterval(fullInterval);
                }
                _partition[i].push_back_Eqclass(fullClass);
                for(uint32_t j = 0; j < place.type->productSize(); j++){
                    _partition[i].push_back_diagonalTuplePos(false);
                }
                _placeQueue.push_back(i);
                _inQueue[i] = true;
            }
        }

        void PartitionBuilder::printPartion() const {
            for(const auto &equivalenceVec : _partition){
                std::cout << "Partition for place " << _places[equivalenceVec.first].name << std::endl;
                std::cout << "Diag variables: (";
                for(auto daigPos : equivalenceVec.second.getDiagonalTuplePositions()){
                    std::cout << daigPos << ",";
                }
                std::cout << ")" << std::endl;
                for (const auto &equivalenceClass : equivalenceVec.second.getEquivalenceClasses()){
                    std::cout << equivalenceClass.toString() << std::endl;
                    
                }
                std::cout << "Diagonal " << equivalenceVec.second.isDiagonal() << std::endl << std::endl;;
            }
        }
        
        bool PartitionBuilder::partitionNet(int32_t timeout) {
            const auto start = std::chrono::high_resolution_clock::now();
            handleLeafTransitions();            
            auto end = std::chrono::high_resolution_clock::now();
            
            while(!_placeQueue.empty() && timeout > 0 && std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < timeout){
                auto placeId = _placeQueue.back();
                _placeQueue.pop_back();
                _inQueue[placeId] = false;

                bool allPositionsDiagonal = true;
                for(auto diag : _partition[placeId].getDiagonalTuplePositions()){
                    if(!diag){
                        allPositionsDiagonal = false;
                        break;
                    }
                }

                if(allPositionsDiagonal || _partition[placeId].getEquivalenceClasses().size() >= _partition[placeId].getEquivalenceClasses().back()._colorType->size(_partition[placeId].getDiagonalTuplePositions())){
                    _partition[placeId].setDiagonal(true);
                }
                if(_placePreTransitionMap.find(placeId) != _placePreTransitionMap.end()){    
                    for(uint32_t transitionId : _placePreTransitionMap.find(placeId)->second){
                        handleTransition(transitionId, placeId);
                    }
                }
                end = std::chrono::high_resolution_clock::now();
            }
            return _placeQueue.empty();         
        }

        void PartitionBuilder::assignColorMap(std::unordered_map<uint32_t, EquivalenceVec> &partition) const{
            for(auto& eqVec : partition){
                if(eqVec.second.isDiagonal()){
                    continue;
                }
                ColorType *colorType = _places[eqVec.first].type;
                for(uint32_t i = 0; i < colorType->size(); i++){ 
                    const Color *color = &colorType->operator[](i);
                    eqVec.second.addColorToEqClassMap(color);                   
                }               
            }
        }

        void PartitionBuilder::handleTransition(uint32_t transitionId, uint32_t postPlaceId){
            const PetriEngine::Colored::Transition &transition = _transitions[transitionId];
            Arc postArc;
            bool arcFound = false;
            for(const auto& outArc : transition.output_arcs){
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

        //Check if a variable appears more than once on the output arc
        //If the place is does not have a product colortype mark the whole place as diagonal, otherwise only the positions
        void PartitionBuilder::checkVarOnArc(const VariableModifierMap &varModifierMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId, bool inputArc){
            for(const auto &varModMap : varModifierMap){
                if(varModMap.second.size() > 1){
                    uint32_t actualSize = 0;
                    std::vector<uint32_t> positions;
                    for(const auto &map : varModMap.second){
                        if(!map.empty()){
                            for(auto position : map){
                                positions.push_back(position.first);
                            }                            
                            actualSize++;
                        }
                    }
                    if(actualSize > 1) {
                        diagonalVars.insert(varModMap.first);
                        if(_partition[placeId].getEquivalenceClasses().back()._colorType->productSize() == 1){
                            _partition[placeId].setDiagonal(true);
                        } else {                            
                            for(auto pos : positions){
                                if(!_partition[placeId].getDiagonalTuplePositions()[pos]){
                                    if(inputArc) addToQueue(placeId);
                                    _partition[placeId].setDiagonalTuplePosition(pos,true);
                                }
                            }
                        }
                    } 
                }
            }
        }
        //Check if the variales on this preArc also appear on other preArcs for the transition
        //and mark them as diagonal if they do
        void PartitionBuilder::checkVarOnInputArcs(const std::unordered_map<uint32_t,PositionVariableMap> &placeVariableMap, const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId){
            for(const auto &placeVariables : placeVariableMap){
                for(const auto &variable : preVarPositionMap){
                    for(const auto &varPosition : placeVariables.second){
                        if(varPosition.second == variable.second){
                            diagonalVars.insert(variable.second);
                            if(_partition[placeId].getEquivalenceClasses().back()._colorType->productSize() == 1){
                                _partition[placeId].setDiagonal(true);
                            } else if(!_partition[placeId].getDiagonalTuplePositions()[variable.first]) {                                      
                                addToQueue(placeId);
                                _partition[placeId].setDiagonalTuplePosition(variable.first,  true);                            
                            } 

                            if(_partition[placeVariables.first].getEquivalenceClasses().back()._colorType->productSize() == 1){
                                _partition[placeVariables.first].setDiagonal(true);
                                addToQueue(placeVariables.first);
                            } else if(!_partition[placeVariables.first].getDiagonalTuplePositions()[varPosition.first]) {
                                addToQueue(placeVariables.first);
                                _partition[placeVariables.first].setDiagonalTuplePosition(varPosition.first, true);
                            }                                     
                            break;                                
                        }
                    }
                    if(_partition[placeId].isDiagonal()){
                        break;
                    }                            
                }
                if(_partition[placeId].isDiagonal()){
                    break;
                }
            }
        }
        //Check if the preArc share variables with the postArc and mark diagonal if the 
        //variable positions are diagonal in the post place
        void PartitionBuilder::markSharedVars(const PositionVariableMap &preVarPositionMap, const PositionVariableMap &varPositionMap, uint32_t postPlaceId, uint32_t prePlaceId){                   
            for(const auto &preVar : preVarPositionMap){
                for(const auto &postVar : varPositionMap){
                    if(preVar.second == postVar.second){
                        if(_partition[postPlaceId].isDiagonal() || _partition[postPlaceId].getDiagonalTuplePositions()[postVar.first]){
                            if(_partition[prePlaceId].getEquivalenceClasses().back()._colorType->productSize() == 1){
                                _partition[prePlaceId].setDiagonal(true);
                            } else if(!_partition[prePlaceId].getDiagonalTuplePositions()[preVar.first]) {
                                addToQueue(prePlaceId);
                                _partition[prePlaceId].setDiagonalTuplePosition(preVar.first, true);
                            }
                        }
                    }
                }
            }
        }
        //Check if any of the variables on the preArc was part of a diagonal constraint in the gaurd
        void PartitionBuilder::checkVarInGuard(const PositionVariableMap &preVarPositionMap, const std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId){
            for(const auto &preVar : preVarPositionMap){
                if(diagonalVars.count(preVar.second)){
                    if(_partition[placeId].getEquivalenceClasses().back()._colorType->productSize() == 1){
                        _partition[placeId].setDiagonal(true);
                        break;
                    } else if(!_partition[placeId].getDiagonalTuplePositions()[preVar.first]) {
                        addToQueue(placeId);
                        _partition[placeId].setDiagonalTuplePosition(preVar.first, true);
                    }                           
                }
            }
        }

        //Check if we have marked all positions in the product type of the place as diagonal
        //and mark the whole place as diagonal if it is the case
        bool PartitionBuilder::checkTupleDiagonal(uint32_t placeId){
            bool allPositionsDiagonal = true;
            for(auto diag : _partition[placeId].getDiagonalTuplePositions()){
                if(!diag){
                    allPositionsDiagonal = false;
                    break;
                }
            }

            if(allPositionsDiagonal){
                _partition[placeId].setDiagonal(true);
                addToQueue(placeId);
                return true;
            }
            return false; 
        }

        bool PartitionBuilder::checkDiagonal(uint32_t placeId){
            if(_partition[placeId].isDiagonal()){
                addToQueue(placeId);
                return true;
            }
            return false;
        }

        void PartitionBuilder::applyNewIntervals(const Arc &inArc, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps){
            //Retrieve the intervals for the current place, 
            //based on the intervals from the postPlace, the postArc, preArc and guard
            auto outIntervals = inArc.expr->getOutputIntervals(varMaps);
            EquivalenceVec newEqVec;
            for(auto& intervalTuple : outIntervals){
                intervalTuple.simplify();
                EquivalenceClass newEqClass = EquivalenceClass(_partition[inArc.place].getEquivalenceClasses().back()._colorType, std::move(intervalTuple));
                newEqVec.push_back_Eqclass(std::move(newEqClass));
            }                    
            newEqVec.setDiagonalTuplePositions(_partition[inArc.place].getDiagonalTuplePositions());                    
            
            //If the prePlace has not been marked as diagonal, then split the current partitions based on the new intervals
            if(splitPartition(std::move(newEqVec), inArc.place)){
                addToQueue(inArc.place);
            }
            _partition[inArc.place].mergeEqClasses();
        }

        void PartitionBuilder::handleTransition(const Transition &transition, const uint32_t postPlaceId, const Arc *postArc) {
            VariableModifierMap varModifierMap;
            PositionVariableMap varPositionMap;
            std::set<const PetriEngine::Colored::Variable *> postArcVars;
            std::set<const PetriEngine::Colored::Variable *> guardVars;
            std::set<const Colored::Variable*> diagonalVars;
            
            postArc->expr->getVariables(postArcVars, varPositionMap, varModifierMap, true);

            checkVarOnArc(varModifierMap, diagonalVars, postPlaceId, false);

            if(transition.guard != nullptr){
                transition.guard->getVariables(guardVars);
            }           
            // we have to copy here, the following loop has the *potential* to modify _partition[postPlaceId]
            const std::vector<Colored::EquivalenceClass> placePartition = _partition[postPlaceId].getEquivalenceClasses();

            //Partition each of the equivalence classes
            for(const auto &eqClass : placePartition){
                auto varMaps = prepareVariables(varModifierMap, eqClass, postArc, postPlaceId);

                //If there are variables in the guard, that doesn't come from the postPlace
                //we give them the full interval
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

                handleInArcs(transition, diagonalVars, varPositionMap, varMaps, postPlaceId);
            }
        }

        void PartitionBuilder::handleInArcs(const Transition &transition, std::set<const Colored::Variable*> &diagonalVars, const PositionVariableMap &varPositionMap, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId){
            std::unordered_map<uint32_t,PositionVariableMap> placeVariableMap;
            for(const auto &inArc : transition.input_arcs){
                //Hack to avoid considering dot places and dealing with retrieving the correct dot pointer
                if(_places[inArc.place].type->getName() == "Dot" || _places[inArc.place].type->getName() == "dot"){
                    _partition[inArc.place].setDiagonal(true);
                }

                if(_partition[inArc.place].isDiagonal()){
                    continue;
                }
                VariableModifierMap preVarModifierMap;
                PositionVariableMap preVarPositionMap;
                std::set<const PetriEngine::Colored::Variable *> preArcVars;
                inArc.expr->getVariables(preArcVars, preVarPositionMap, preVarModifierMap, true);
                checkVarOnInputArcs(placeVariableMap, preVarPositionMap, diagonalVars, inArc.place);
                placeVariableMap[inArc.place] = preVarPositionMap;
                if(checkDiagonal(inArc.place)) continue;

                markSharedVars(preVarPositionMap, varPositionMap, postPlaceId, inArc.place);
                if(checkDiagonal(inArc.place)) continue;
                checkVarOnArc(preVarModifierMap, diagonalVars, inArc.place, true);
                if(checkDiagonal(inArc.place)) continue;

                checkVarInGuard(preVarPositionMap, diagonalVars, inArc.place);
                if(checkDiagonal(inArc.place)) continue;

                _partition[inArc.place].mergeEqClasses();
                if(_partition[inArc.place].getEquivalenceClasses().size() >= _partition[inArc.place].getEquivalenceClasses().back()._colorType->size(_partition[inArc.place].getDiagonalTuplePositions())){
                    _partition[inArc.place].setDiagonal(true);
                    continue;
                }

                if(checkTupleDiagonal(inArc.place)){
                    continue;
                }                

                applyNewIntervals(inArc, varMaps);
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
                    const auto &ec1 = equivalenceVec.getEquivalenceClasses()[ecPos1];
                    const auto &ec2 = _partition[placeId].getEquivalenceClasses()[ecPos2];
                    const auto &rightSubtractEc = ec1.subtract(ec2, equivalenceVec.getDiagonalTuplePositions());
                    const auto &leftSubtractEc = ec2.subtract(ec1, _partition[placeId].getDiagonalTuplePositions());                    

                    equivalenceVec.erase_Eqclass(ecPos1);
                    _partition[placeId].erase_Eqclass(ecPos2);

                    if(!intersection.isEmpty()){
                        _partition[placeId].push_back_Eqclass(intersection);
                        intersection._colorIntervals._intervals.clear();
                    }
                    if(!leftSubtractEc.isEmpty()){
                        _partition[placeId].push_back_Eqclass(leftSubtractEc);
                        split = true;
                    }
                    if(!rightSubtractEc.isEmpty()){
                        equivalenceVec.push_back_Eqclass(rightSubtractEc);
                    }                                     
                }
            }
            return split;
        }

        bool PartitionBuilder::findOverlap(const EquivalenceVec &equivalenceVec1, const EquivalenceVec &equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection) const{
            for(uint32_t i = 0; i < equivalenceVec1.getEquivalenceClasses().size(); i++){
                for(uint32_t j = 0; j < equivalenceVec2.getEquivalenceClasses().size(); j++){
                    const auto &ec = equivalenceVec1.getEquivalenceClasses()[i];
                    const auto &ec2 = equivalenceVec2.getEquivalenceClasses()[j];
                    
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

        std::vector<VariableIntervalMap> 
        PartitionBuilder::prepareVariables(
                    const VariableModifierMap &varModifierMap, 
                    const EquivalenceClass& eqClass , const Arc *arc, uint32_t placeId){
            std::vector<VariableIntervalMap> varMaps;
            VariableIntervalMap varMap;
            varMaps.push_back(varMap);
            std::unordered_map<uint32_t, ArcIntervals> placeArcIntervals;
            ColorFixpoint postPlaceFixpoint;
            postPlaceFixpoint.constraints = eqClass._colorIntervals; 
            ArcIntervals newArcInterval(&postPlaceFixpoint, varModifierMap);
            uint32_t index = 0;

            arc->expr->getArcIntervals(newArcInterval, postPlaceFixpoint, &index, 0);
            placeArcIntervals[placeId] = std::move(newArcInterval);
            intervalGenerator.getVarIntervals(varMaps, placeArcIntervals);

            return varMaps;                
        }

        void PartitionBuilder::handleLeafTransitions(){
            for(uint32_t i = 0; i < _transitions.size(); i++){
                const Transition &transition = _transitions[i];
                if(transition.output_arcs.empty() && !transition.input_arcs.empty()){
                    handleTransition(transition, transition.input_arcs.back().place, &transition.input_arcs.back());
                }
            }
        }
    }    
}
