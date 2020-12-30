/*
 * File:   ColoredPetriNetBuilder.cpp
 * Author: Klostergaard
 * 
 * Created on 17. februar 2018, 16:25
 */

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include <chrono>

namespace PetriEngine {
    ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
    }

    ColoredPetriNetBuilder::ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig) {
    }

    ColoredPetriNetBuilder::~ColoredPetriNetBuilder() {
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, int tokens, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addPlace(name, tokens, x, y);
        }
    }

    void ColoredPetriNetBuilder::addPlace(const std::string& name, Colored::ColorType* type, Colored::Multiset&& tokens, double x, double y) {
        if(_placenames.count(name) == 0)
        {
            uint32_t next = _placenames.size();
            _places.emplace_back(Colored::Place {name, type, tokens});
            _placenames[name] = next;

            //set up place color fix points and initialize queue
            if (!tokens.empty()) {
                _placeFixpointQueue.emplace_back(next);
            }

            Reachability::intervalTuple_t placeConstraints;
            Colored::ColorFixpoint colorFixpoint = {placeConstraints, !tokens.empty(), (uint32_t) type->productSize()};

            if(tokens.size() == type->size()){
                colorFixpoint.constraints.addInterval(type->getFullInterval());
            } else {
                uint32_t index = 0;
                for (auto colorPair : tokens) {
                    Reachability::interval_t tokenConstraints;
                    colorPair.first->getColorConstraints(&tokenConstraints, &index);

                    colorFixpoint.constraints.addInterval(tokenConstraints);
                    index = 0;
                }
            }
         
            _placeColorFixpoints.push_back(colorFixpoint);            
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, double x, double y) {
        if (!_isColored) {
            _ptBuilder.addTransition(name, x, y);
        }
    }

    void ColoredPetriNetBuilder::addTransition(const std::string& name, const Colored::GuardExpression_ptr& guard, double x, double y) {
        if(_transitionnames.count(name) == 0)
        {
            uint32_t next = _transitionnames.size();
            _transitions.emplace_back(Colored::Transition {name, guard});
            _transitionnames[name] = next;
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, int weight) {
        if (!_isColored) {
            _ptBuilder.addInputArc(place, transition, inhibitor, weight);
        }
    }

    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr) {
        addArc(place, transition, expr, true);
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, int weight) {
        if (!_isColored) {
            _ptBuilder.addOutputArc(transition, place, weight);
        }
    }

    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, const Colored::ArcExpression_ptr& expr) {
        addArc(place, transition, expr, false);
    }

    void ColoredPetriNetBuilder::addArc(const std::string& place, const std::string& transition, const Colored::ArcExpression_ptr& expr, bool input) {
        if(_transitionnames.count(transition) == 0)
        {
            std::cout << "Transition '" << transition << "' not found. Adding it." << std::endl;
            addTransition(transition,0.0,0.0);
        }
        if(_placenames.count(place) == 0)
        {
            std::cout << "Place '" << place << "' not found. Adding it." << std::endl;
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];

        assert(t < _transitions.size());
        assert(p < _places.size());

        if (input) _placePostTransitionMap[p].emplace_back(t);

        Colored::Arc arc;
        arc.place = p;
        arc.transition = t;
        assert(expr != nullptr);
        arc.expr = std::move(expr);
        arc.input = input;
        input? _transitions[t].input_arcs.push_back(std::move(arc)): _transitions[t].output_arcs.push_back(std::move(arc));
    }

    void ColoredPetriNetBuilder::addColorType(const std::string& id, Colored::ColorType* type) {
        _colors[id] = type;
    }

    void ColoredPetriNetBuilder::sort() {

    }

    void ColoredPetriNetBuilder::printPlaceTable() {
        for (auto place: _places) {
            auto placeID = _placenames[place.name];
            auto placeColorFixpoint = _placeColorFixpoints[placeID];
            std::cout << "Place: " << place.name << " in queue: " << placeColorFixpoint.inQueue  << " with colortype " << place.type->getName() << std::endl;

            for(auto fixpointPair : placeColorFixpoint.constraints._intervals) {
                std::cout << "[";
                for(auto range : fixpointPair._ranges) {
                    std::cout << range._lower << "-" << range._upper << ", ";
                }
                std::cout << "]"<< std::endl;                    
            }

            std::cout << std::endl;
        }
        

    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint(uint32_t max_intervals) {

        auto start = std::chrono::high_resolution_clock::now();
        std::chrono::_V2::system_clock::time_point end = std::chrono::high_resolution_clock::now();
        while(!_placeFixpointQueue.empty() && std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < 10){
            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();
            _placeColorFixpoints[currentPlaceId].inQueue = false;
 
            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];
            // std::cout << "Place " << _places[currentPlaceId].name << std::endl;

            for (uint32_t transitionId : connectedTransitions) {
                
                Colored::Transition& transition = _transitions[transitionId];
                if (transition.considered) continue;
                bool transitionActivated = true;
                //maybe this can be avoided with a better implementation
                transition.variableMaps.clear();

                // std::cout << "Transtition " << transition.name << std::endl;

                if(!_arcIntervals.count(transitionId)){
                    _arcIntervals[transitionId] = setupTransitionVars(transition);
                }

                // for(auto& arcInterval : _arcIntervals[transitionId]){
                //     arcInterval.second.resetIntervals();
                // } 
                           
                processInputArcs(transition, currentPlaceId, transitionId, transitionActivated, max_intervals);
                

                
                if (transitionActivated) {
                    processOutputArcs(transition);
                }
                              
            }
            end = std::chrono::high_resolution_clock::now();
        }
        
        if(_placeFixpointQueue.empty()){
            _fixpointDone = true;
        }
        _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        // std::cout << "Total time " << totalinputtime << std::endl;
        // std::cout << "Total time2 " << totalinputtime2 << std::endl;
        
        //printPlaceTable();
        //We should not need to keep colors in places after we have found fixpoint
        _placeColorFixpoints.clear();
    }

    std::unordered_map<uint32_t, Colored::ArcIntervals> ColoredPetriNetBuilder::setupTransitionVars(Colored::Transition transition){
        std::unordered_map<uint32_t, Colored::ArcIntervals> res;
        for(auto arc : transition.input_arcs){
            std::set<const Colored::Variable *> variables;
            std::unordered_map<uint32_t, const Colored::Variable *> varPositions;
            std::unordered_map<const Colored::Variable *, std::vector<std::unordered_map<uint32_t, int32_t>>> varModifiersMap;
            arc.expr->getVariables(variables, varPositions, varModifiersMap);

            Colored::ArcIntervals newArcInterval(&_placeColorFixpoints[arc.place], varModifiersMap);
            res[arc.place] = newArcInterval;               
        }
        return res;
    }



    void ColoredPetriNetBuilder::processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals) {
        
        for (auto arc : transition.input_arcs) {
            PetriEngine::Colored::ColorFixpoint& curCFP = _placeColorFixpoints[arc.place];
            
            
            curCFP.constraints.restrict(max_intervals);
             
            // if(transition.name == "identity"){
            //      std::cout << "Cur place point " << arc.place << " and arc expression " << arc.expr.get()->toString() << std::endl;
            // curCFP.constraints.print();
            // }
           

            Colored::ArcIntervals& arcInterval = _arcIntervals[transitionId][arc.place];
            uint32_t index = 0;
            arcInterval._intervalTupleVec.clear();

            if(!arc.expr->getArcIntervals(arcInterval, curCFP, &index, 0)){
                std::cout << "Failed to get arc intervals" << std::endl;
                transitionActivated = false;
                return;
            } 
            // if(transition.name == "identity"){
            //     std::cout << "Arc interval " << std::endl;
            // arcInterval.print();
            // }
            
        }
        // auto startinput = std::chrono::high_resolution_clock::now();
        // std::cout << "Getting var intervals" << std::endl;
        if(getVarIntervals(transition.variableMaps, transitionId)){
            // auto endinput = std::chrono::high_resolution_clock::now();
            // totalinputtime += (std::chrono::duration_cast<std::chrono::microseconds>(endinput - startinput).count())*0.000001;
            // if(transition.name == "identity"){
            //     std::cout << transition.name << " var intervals" << std::endl;
            //     for (auto pair : transition.variableMaps){
            //         std::cout << "Var set:" << std::endl;
            //         for(auto varIntervalsPair : pair){
            //             std::cout << varIntervalsPair.first->name << std::endl;
            //             varIntervalsPair.second.print();
            //         }
            //     }
            // }
            
            if(transition.guard != nullptr) {
                // std::cout << "applying guard" << std::endl;
                // auto startinput2 = std::chrono::high_resolution_clock::now(); 
                transition.guard->restrictVars(transition.variableMaps);
                // auto endinput2 = std::chrono::high_resolution_clock::now();
                // totalinputtime2 += (std::chrono::duration_cast<std::chrono::microseconds>(endinput2 - startinput2).count())*0.000001;
                
                std::vector<std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Reachability::intervalTuple_t>> newVarmaps;
                for(auto& varMap : transition.variableMaps){
                    bool validVarMap = true;      
                    for(auto varPair : varMap){
                        if(!varPair.second.hasValidIntervals()){
                            validVarMap = false;
                            break;
                        }
                    } 
                    if(validVarMap){
                        newVarmaps.push_back(std::move(varMap));
                    }                   
                }

                transition.variableMaps = std::move(newVarmaps);

                if(transition.variableMaps.empty()){
                    std::cout << "Guard restrictions removed all valid intervals" << std::endl;
                    transitionActivated = false;
                    return;
                }
            }
        } else {
            // auto endinput = std::chrono::high_resolution_clock::now();
            // totalinputtime += (std::chrono::duration_cast<std::chrono::microseconds>(endinput - startinput).count())*0.000001;
            std::cout << "getting var intervals failed " << std::endl;
            transitionActivated = false;
        }                                            
    }


    bool ColoredPetriNetBuilder::getVarIntervals(std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>>& variableMaps, uint32_t transitionId){
        for(auto& placeArcIntervals : _arcIntervals[transitionId]){            
            for(uint32_t j = 0; j < placeArcIntervals.second._intervalTupleVec.size(); j++){
                uint32_t intervalTupleSize = placeArcIntervals.second._intervalTupleVec[j].size();
                //If we have not found intervals for any place yet, we fill the intervals from this place
                //Else we restrict the intervals we already found to only keep those that can also be matched in this place
                if(variableMaps.empty()){                    
                    
                    for(uint32_t i = 0; i < intervalTupleSize; i++){
                        std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t> localVarMap;
                        bool validInterval = true;
                        auto interval = &placeArcIntervals.second._intervalTupleVec[j]._intervals[i];
                       
                        for(auto pair : placeArcIntervals.second._varIndexModMap){
                            Reachability::intervalTuple_t varIntervals;
                            std::vector<Colored::ColorType*> varColorTypes;
                            pair.first->colorType->getColortypes(varColorTypes);
                            getArcVarIntervals(varIntervals, pair.second[j], interval, varColorTypes); 

                            if(placeArcIntervals.second._intervalTupleVec.size() > 1 && pair.second[j].empty()){
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
                                                  
                } else {
                    
                    std::vector<std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t>> newVarMapVec;
                      
                    for(auto& varMap : variableMaps){       
                        
                        for(uint32_t i = 0; i < intervalTupleSize; i++){

                            std::unordered_map<const Colored::Variable *, Reachability::intervalTuple_t> localVarMap;
                            bool allVarsAssigned = true;
                            auto interval = &placeArcIntervals.second._intervalTupleVec[j]._intervals[i];
                            
                            for(auto& pair : placeArcIntervals.second._varIndexModMap){
                                
                                Reachability::intervalTuple_t varIntervals; 
                                std::vector<Colored::ColorType*> varColorTypes;
                                pair.first->colorType->getColortypes(varColorTypes);
                                
                                getArcVarIntervals(varIntervals, pair.second[j], interval, varColorTypes);                          
                                
                                if (placeArcIntervals.second._intervalTupleVec.size() > 1 && pair.second[j].empty()) {
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

    void ColoredPetriNetBuilder::getArcVarIntervals(Reachability::intervalTuple_t& varIntervals, std::unordered_map<uint32_t, int32_t> modIndexMap, PetriEngine::Reachability::interval_t *interval, std::vector<Colored::ColorType*> varColorTypes){
        for(auto& posModPair : modIndexMap){
            auto intervals = getIntervalsFromInterval(interval, posModPair.first, posModPair.second, varColorTypes);

            if(varIntervals._intervals.empty()){
                for(auto& interval : intervals){
                    varIntervals.addInterval(std::move(interval));
                }
            } else {
                Reachability::intervalTuple_t newVarIntervals; 
                for(uint32_t i = 0; i < varIntervals.size(); i++){
                    auto varInterval = &varIntervals[i];
                    for(auto& interval : intervals){
                        auto overlap = varInterval->getOverlap(std::move(interval));
                        if(overlap.isSound()){ 
                            newVarIntervals.addInterval(std::move(overlap));
                            break;
                        }                                                
                    }                                   
                }                                                                                        
                varIntervals = std::move(newVarIntervals);                                          
            }     
        } 
    }

    std::vector<Reachability::interval_t> ColoredPetriNetBuilder::getIntervalsFromInterval(Reachability::interval_t *interval, uint32_t varPosition, int32_t varModifier, std::vector<Colored::ColorType*> varColorTypes){
        std::vector<Reachability::interval_t> varIntervals;
        Reachability::interval_t firstVarInterval;
        varIntervals.push_back(firstVarInterval);
        for(uint32_t i = varPosition; i < varPosition + varColorTypes.size(); i++){
            auto ctSize = varColorTypes[i - varPosition]->size();
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
                    std::vector<Reachability::interval_t> newIntervals;
                    for (auto& varInterval : varIntervals){
                        Reachability::interval_t newVarInterval = varInterval;                                  
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

    void ColoredPetriNetBuilder::processOutputArcs(Colored::Transition& transition) {
        bool transitionHasVarOutArcs = false;

        // std::cout << "Transistion " << transition.name << " activated" << std::endl;
        // std::cout << "Vars bound to " << std::endl;
        // for(auto varmap : transition.variableMap){
        //     std::cout << "Var set:" << std::endl;
        //     for(auto pair : varmap){
        //         std::cout << pair.first->name << std::endl;
        //         pair.second.print();
        //     }
        // }

        for (auto& arc : transition.output_arcs) {
            Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];

            //used to check if colors are added to the place. The total distance between upper and
            //lower bounds should grow when more colors are added and as we cannot remove colors this
            //can be checked by summing the differences
            uint32_t colorsBefore = 0;
            for (auto interval : placeFixpoint.constraints._intervals) {
                uint32_t intervalColors = 1;
                for(auto range: interval._ranges) {
                    intervalColors *= 1+  range._upper - range._lower;
                }  
                colorsBefore += intervalColors;              
            }
                
            std::set<const Colored::Variable *> variables;
            arc.expr->getVariables(variables);           

            if (!variables.empty()) {
                transitionHasVarOutArcs = true;
            }
            // std::cout << "Getting output intervals" << std::endl;
            auto intervals = arc.expr->getOutputIntervals(transition.variableMaps);
            // std::cout << "now simplifying" << std::endl;
            intervals.simplify();

            // std::cout << "Output intervals for arc expr " << arc.expr.get()->toString() << std::endl;
            // intervals.print();

            for(auto interval : intervals._intervals){
                placeFixpoint.constraints.addInterval(interval);    
            }

            if (!placeFixpoint.inQueue) {
                uint32_t colorsAfter = 0;
                for (auto interval : placeFixpoint.constraints._intervals) {
                    uint32_t intervalColors = 1;
                    for(auto range: interval._ranges) {
                        intervalColors *= 1+  range._upper - range._lower;
                    }  
                    colorsAfter += intervalColors;                        
                }
                // std::cout << "Colors before: " << colorsBefore << " and after: " << colorsAfter << " for place " << arc.place << std::endl;
                if (colorsAfter > colorsBefore) {
                    _placeFixpointQueue.push_back(arc.place);
                    placeFixpoint.inQueue = true;
                }
            }                   
        }

        //If there are no variables among the out arcs of a transition 
        // and it has been activated, there is no reason to cosider it again
        if(!transitionHasVarOutArcs) {
            transition.considered = true;
        }
    }

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();
            //for (auto& place : _places) {
            //    unfoldPlace(place);
            //}

            std::cout << "Unfolding " << _fixpointDone << std::endl;

            for (auto& transition : _transitions) {
                unfoldTransition(transition);
            }
            for (auto& place : _places) {
               handleOrphanPlace(place);
            }
            _unfolded = true;
            auto end = std::chrono::high_resolution_clock::now();
            _time = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        }
        return _ptBuilder;
    }
    //Due to the way we unfold places, we only unfold palces connected to an arc (which makes sense)
    //However, in queries asking about orphan places it cannot find these, as they have not been unfolded
    //so we make a placeholder place which just has tokens equal to the number of colored tokens
    //Ideally, orphan places should just be translated to a constant in the query
    void ColoredPetriNetBuilder::handleOrphanPlace(Colored::Place& place) {
        if(_ptplacenames.count(place.name) <= 0){
            
            std::string name = place.name + "_" + std::to_string(place.marking.size());
            _ptBuilder.addPlace(name, place.marking.size(), 0.0, 0.0);
            _ptplacenames[place.name][0] = std::move(name);
        }
        
        //++_nptplaces;
        
    }

    void ColoredPetriNetBuilder::unfoldPlace(Colored::Place& place) {
        // auto placeId = _placenames[place.name];
        // auto placeFixpoint = _placeColorFixpoints[placeId];
        // for (size_t i = placeFixpoint.interval_lower; i <= placeFixpoint.interval_upper; ++i) {
        //     std::string name = place.name + "_" + std::to_string(i);
        //     const Colored::Color* color = &place.type->operator[](i);
        //     _ptBuilder.addPlace(name, place.marking[color], 0.0, 0.0);
        //     _ptplacenames[place.name][color->getId()] = std::move(name);
        //     ++_nptplaces;
        // }
    }

    void ColoredPetriNetBuilder::unfoldTransition(Colored::Transition& transition) {
        if(_fixpointDone){
            BindingGenerator gen(transition, _colors);
            size_t i = 0;
            for (auto b : gen) {

                //Print all bindings
                // std::cout << "Unfolding " << transition.name << " to the binding: "<< std::endl;
                // for (auto test : b){
                //     std::cout << "Binding '" << test.first->name << "\t" << test.second->getColorName() << "' in bindings." << std::endl;
                // }
                // std::cout << std::endl;
                
                
                std::string name = transition.name + "_" + std::to_string(i++);
                _ptBuilder.addTransition(name, 0.0, 0.0);
                _pttransitionnames[transition.name].push_back(name);
                ++_npttransitions;
                for (auto& arc : transition.input_arcs) {
                    unfoldArc(arc, b, name, true );
                }
                for (auto& arc : transition.output_arcs) {
                    unfoldArc(arc, b, name, false);
                }
            }
        } else {
            NaiveBindingGenerator gen(transition, _arcs, _colors);
            size_t i = 0;
            for (auto b : gen) {

                //Print all bindings
                // std::cout << "Unfolding " << transition.name << " to the binding: "<< std::endl;
                // for (auto test : b){
                //     std::cout << "Binding '" << test.first->name << "\t" << test.second->getColorName() << "' in bindings." << std::endl;
                // }
                // std::cout << std::endl;
                
                
                std::string name = transition.name + "_" + std::to_string(i++);
                _ptBuilder.addTransition(name, 0.0, 0.0);
                _pttransitionnames[transition.name].push_back(name);
                ++_npttransitions;
                for (auto& arc : transition.input_arcs) {
                    unfoldArc(arc, b, name, true );
                }
                for (auto& arc : transition.output_arcs) {
                    unfoldArc(arc, b, name, false);
                }
            }
        }
        
    }

    void ColoredPetriNetBuilder::unfoldArc(Colored::Arc& arc, Colored::ExpressionContext::BindingMap& binding, std::string& tName, bool input) {
        Colored::ExpressionContext context {binding, _colors};
        auto ms = arc.expr->eval(context);       

        for (const auto& color : ms) {
            if (color.second == 0) {
                continue;
            }
            const PetriEngine::Colored::Place& place = _places[arc.place];
            const std::string& pName = _ptplacenames[place.name][color.first->getId()];
            if (pName.empty()) {
                
                std::string name = place.name + "_" + std::to_string(color.first->getId());
                _ptBuilder.addPlace(name, place.marking[color.first], 0.0, 0.0);
                _ptplacenames[place.name][color.first->getId()] = std::move(name);
                ++_nptplaces;                
            }
            if (arc.input) {
                _ptBuilder.addInputArc(pName, tName, false, color.second);
            } else {
                _ptBuilder.addOutputArc(tName, pName, color.second);
            }
            ++_nptarcs;
        }
    }

    PetriNetBuilder& ColoredPetriNetBuilder::stripColors() {
        if (_unfolded) assert(false);
        if (_isColored && !_stripped) {
            for (auto& place : _places) {
                _ptBuilder.addPlace(place.name, place.marking.size(), 0.0, 0.0);
            }

            for (auto& transition : _transitions) {
                _ptBuilder.addTransition(transition.name, 0.0, 0.0);
                for (auto& arc : transition.input_arcs) {
                    try {
                        _ptBuilder.addInputArc(_places[arc.place].name, _transitions[arc.transition].name, false,
                                                arc.expr->weight());
                    } catch (Colored::WeightException& e) {
                        std::cerr << "Exception on input arc: " << arcToString(arc) << std::endl;
                        std::cerr << "In expression: " << arc.expr->toString() << std::endl;
                        std::cerr << e.what() << std::endl;
                        exit(ErrorCode);
                    }
                }
                for (auto& arc : transition.output_arcs) {
                    try {
                        _ptBuilder.addOutputArc(_transitions[arc.transition].name, _places[arc.place].name,
                                                arc.expr->weight());
                    } catch (Colored::WeightException& e) {
                        std::cerr << "Exception on output arc: " << arcToString(arc) << std::endl;
                        std::cerr << "In expression: " << arc.expr->toString() << std::endl;
                        std::cerr << e.what() << std::endl;
                        exit(ErrorCode);
                    }
                }
            }

            _stripped = true;
            _isColored = false;
        }

        return _ptBuilder;
    }

    std::string ColoredPetriNetBuilder::arcToString(Colored::Arc& arc) const {
        return !arc.input ? "(" + _transitions[arc.transition].name + ", " + _places[arc.place].name + ")" :
               "(" + _places[arc.place].name + ", " + _transitions[arc.transition].name + ")";
    }

    BindingGenerator::Iterator::Iterator(BindingGenerator* generator)
            : _generator(generator)
    {
    }

    bool BindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool BindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    BindingGenerator::Iterator& BindingGenerator::Iterator::operator++() {
        if (_generator->_isDone) {
            _generator = nullptr;
        } else {
            _generator->nextBinding();
            if (_generator->_isDone) {
                _generator = nullptr;
            }
        }
        return *this;
    }

    const Colored::ExpressionContext::BindingMap BindingGenerator::Iterator::operator++(int) {
        auto prev = _generator->currentBinding();
        ++*this;
        return prev;
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::Iterator::operator*() {
        return _generator->currentBinding();
    }
    BindingGenerator::BindingGenerator(Colored::Transition& transition,
        ColorTypeMap& colorTypes)
    : _colorTypes(colorTypes), _transition(transition)
    {
        _isDone = false;
        _noValidBindings = false;
        _nextIndex = 0;
        _expr = _transition.guard;

        //combine varmaps
        // for(uint i = 1; i < _transition.variableMaps.size(); i++){
        //     for(auto varPair : transition.variableMaps[i]){
        //         for(auto interval : varPair.second._intervals){
        //             transition.variableMaps[0][varPair.first].addInterval(interval);
        //         }                
        //     }
        // }

        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (auto arc : _transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : _transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }

        if(_transition.name == "identity"){
            std::cout << _transition.name << " varmap size " << _transition.variableMaps.size() << std::endl;
            for(auto varMap : _transition.variableMaps){
                std::cout << "Var set:" << std::endl;
                for(auto pair : varMap){
                    std::cout << pair.first->name << "\t";
                    for(auto interval : pair.second._intervals){
                        interval.print();
                        std::cout << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        
        for (auto var : variables) {
            if(_transition.variableMaps.empty() || _transition.variableMaps[_nextIndex][var]._intervals.empty()){
                _noValidBindings = true;
                break;
            }
            auto color = var->colorType->getColor(_transition.variableMaps[_nextIndex][var].getFirst().getLowerIds());
            _bindings[var] = color;
        }
        
        if (!_noValidBindings && !eval())
            nextBinding();
    }


    bool BindingGenerator::eval() {
        // std::cout << "testing for " << _transition.name << std::endl;
        // for (auto test : _bindings){
        //     std::cout << "Binding '" << test.first->name << "\t" << test.second->getColorName() << "' in bindings." << std::endl;
        // }
        // std::cout << std::endl;
        if (_expr == nullptr)
            return true;

        Colored::ExpressionContext context {_bindings, _colorTypes};
        return _expr->eval(context);
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            bool next = true;
            for (auto& _binding : _bindings) {
                auto varInterval = _transition.variableMaps[_nextIndex][_binding.first];
                std::vector<uint32_t> colorIds;
                _binding.second->getTupleId(&colorIds);
                auto nextIntervalBinding = varInterval.isRangeEnd(colorIds);

                if (nextIntervalBinding.size() == 0){
                    _binding.second = &_binding.second->operator++();
                    next = false;
                    break;                    
                } else {
                    _binding.second = _binding.second->getColorType()->getColor(nextIntervalBinding.getLowerIds());
                    if(!nextIntervalBinding.equals(varInterval.getFirst())){
                        next = false;
                        break;
                    }                
                }
            }
            if(next){
                _nextIndex++;
                if(isInitial()){
                    _isDone = true;
                    break;
                }
                for(auto& _binding : _bindings){
                    _binding.second =  _binding.second->getColorType()->getColor(_transition.variableMaps[_nextIndex][_binding.first].getFirst().getLowerIds());
                }
            }
                 
            test = eval();
        
        }
        
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::currentBinding() {
        return _bindings;
    }

    bool BindingGenerator::isInitial() const{        
        return _nextIndex >= _transition.variableMaps.size();
    }

    BindingGenerator::Iterator BindingGenerator::begin() {
        if(_noValidBindings || _isDone){
            return {nullptr};
        }
        return {this};
    }

    BindingGenerator::Iterator BindingGenerator::end() {
        return {nullptr};
    }




    NaiveBindingGenerator::Iterator::Iterator(NaiveBindingGenerator* generator)
            : _generator(generator)
    {
    }

    bool NaiveBindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool NaiveBindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    NaiveBindingGenerator::Iterator& NaiveBindingGenerator::Iterator::operator++() {
        _generator->nextBinding();
        if (_generator->isInitial()) _generator = nullptr;
        return *this;
    }

    const Colored::ExpressionContext::BindingMap NaiveBindingGenerator::Iterator::operator++(int) {
        auto prev = _generator->currentBinding();
        ++*this;
        return prev;
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::Iterator::operator*() {
        return _generator->currentBinding();
    }

    NaiveBindingGenerator::NaiveBindingGenerator(Colored::Transition& transition,
            const std::vector<Colored::Arc>& arcs,
            ColorTypeMap& colorTypes)
        : _colorTypes(colorTypes)
    {
        _expr = transition.guard;
        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (auto arc : transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto var : variables) {
            _bindings[var] = &var->colorType->operator[](0);
        }
        
        if (!eval())
            nextBinding();
    }

    bool NaiveBindingGenerator::eval() {
        if (_expr == nullptr)
            return true;

        Colored::ExpressionContext context {_bindings, _colorTypes};
        return _expr->eval(context);
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            for (auto& _binding : _bindings) {
                _binding.second = &_binding.second->operator++();
                if (_binding.second->getId() != 0) {
                    break;
                }
            }

            if (isInitial())
                break;

            test = eval();
        }
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& NaiveBindingGenerator::currentBinding() {
        return _bindings;
    }

    bool NaiveBindingGenerator::isInitial() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != 0) return false;
        }
        return true;
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::begin() {
        return {this};
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::end() {
        return {nullptr};
    }

}

