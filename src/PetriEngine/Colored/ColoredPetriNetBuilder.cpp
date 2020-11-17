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

            
            Reachability::rangeInterval_t placeConstraints;
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
                
                colorFixpoint.constraints.mergeIntervals();
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
        arc.activatable = false;
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
            
            //uint32_t index = 0;

            // for (auto color : *place.type) {
            //     std::cout << "Color " << color.toString() << " has index " << index << std::endl;
            //     index++;
            // }
            for(auto fixpointPair : placeColorFixpoint.constraints._ranges) {
                std::cout << "[";
                for(auto range : fixpointPair._ranges) {
                    std::cout << range._lower << "-" << range._upper << ", ";
                }
                std::cout << "]"<< std::endl;                    
            }

            std::cout << std::endl;
        }
        

    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint() {

        auto start = std::chrono::high_resolution_clock::now();
        
        while(!_placeFixpointQueue.empty()){
            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();

            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];

            for (uint32_t transitionId : connectedTransitions) {
                Colored::Transition& transition = _transitions[transitionId];
                if (transition.considered) break;
                bool transitionActivated = true;

                std::cout << "Transition " << transition.name << std::endl;
                
                processInputArcs(transition, currentPlaceId, transitionActivated);

                /*if(transition.name == "I_free"){
                    cout << "transition activated: " << transitionActivated << " for place " << _places[currentPlaceId].name << endl;
                }*/
                
                if (transitionActivated) {
                    processOutputArcs(transition);
                }              
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        printPlaceTable();
        //We should not need to keep colors in places after we have found fixpoint
        _placeColorFixpoints.clear();
    }

    void ColoredPetriNetBuilder::processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, bool &transitionActivated) {

        PetriEngine::Colored::ColorFixpoint& colorfixpoint = _placeColorFixpoints[currentPlaceId];
        colorfixpoint.inQueue = false;

        bool curPlaceArcProcessed = false;


        // std::cout << "transition " << transition.name << " and current place " << std::endl;
        // colorfixpoint.constraints.print();
        
        std::set<Colored::Variable *> transitionVars;
        std::unordered_map<string, std::set<uint32_t>> varGuardPositions;
        std::unordered_map<Colored::Variable *, std::vector<std::pair<uint32_t, int32_t>>>  varModifierMap;
        if (transition.guard != nullptr) {
            transition.guard->getVariables(transitionVars, varGuardPositions, varModifierMap);
        }
        
        for (auto& arc : transition.input_arcs) {
            /*if(transition.name == "I_free"){
                std::cout << "Arc " << arc.place << "->" << transition.name << " with expr: " << arc.expr->toString() << std::endl;

            }*/

            //Once we have considered the arc connecting to the current place 
            //we can break if transitions are not activated
            if (!transitionActivated && curPlaceArcProcessed) {
                break;
            }

            if (arc.place == currentPlaceId) {
                bool succes = true;
                std::set<Colored::Variable *> variables;
                std::unordered_map<string, std::set<uint32_t>> varPositions;
                std::unordered_map<Colored::Variable *, std::vector<std::pair<uint32_t, int32_t>>>  varModifierMap;
                arc.expr->getVariables(variables, varPositions, varModifierMap);

                //std::cout << "Found " << variables.size() << " vars out here" << std::endl;

                if (!variables.empty()) {                            

                    for (auto var : variables) {
                        //std::cout << "Found " << var->name << " for transition " << transition.name << std::endl;
                        
                        auto varIndexes = varPositions[var->name];                        

                        // find way to not always compute the guard
                        // Right now we are not maintaining place constraints from other places using the same variable
                        Reachability::rangeInterval_t guardRangeInterval;
                        Colored::VariableInterval guardVarInterval(var, guardRangeInterval);                                                      

                        collectVarPlaceRestrictions(guardVarInterval, colorfixpoint, varIndexes, varModifierMap);

                        if (transition.guard != nullptr && transitionVars.find(var) != transitionVars.end()) {
                            transition.guard->restrictVar(&guardVarInterval, &transition.variableIntervals);
                        }                        

                        if (!guardVarInterval.hasValidIntervals()) {
                            //If the arc connected to the place under consideration cannot be activated,
                            //then there is no reason to keep checking
                            transitionActivated = false; 
                            succes = false;
                            break;
                        }

                        transition.variableIntervals[var->name] = guardVarInterval;
                        transition.variableIntervals[var->name]._ranges.mergeIntervals();
                    } 
                }                           
                std::unordered_map<const Colored::Color*, std::vector<uint32_t>> constantMap;
                uint32_t index = 0;
                arc.expr->getConstants(constantMap, index);
                
                for (auto constPair : constantMap) {
                    if(!colorfixpoint.constainsColor(constPair)) {
                        //If the arc connected to the place under consideration cannot be activated,
                        //then there is no reason to keep checking
                         //cout << "could not activate arc in these corlors " << endl;
                        transitionActivated = false;
                        succes = false;
                        break;
                    }
                }
                arc.activatable = succes;                                    
                
                curPlaceArcProcessed = true;
            } else if (!arc.activatable) {
                transitionActivated = false;
            }                
        }
    }

    void ColoredPetriNetBuilder::processOutputArcs(Colored::Transition& transition) {
        bool transitionHasVarOutArcs = false;
        for (auto& arc : transition.output_arcs) {
            Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];

            //std::cout << "OUT for arc " << arc.expr.get()->toString() <<  " and transition " << transition.name << std::endl;

            //used to check if colors are added to the place. The total distance between upper and
            //lower bounds should grow when more colors are added and as we cannot remove colors this
            //can be checked by summing the differences
            uint32_t colorsBefore = 0;
            for (auto interval : placeFixpoint.constraints._ranges) {
                for(auto range: interval._ranges) {
                    colorsBefore += 1+  range._upper - range._lower;
                }                
            }
                
            std::set<Colored::Variable *> variables;
            std::unordered_map<string, std::set<uint32_t>> varPositions;
            std::unordered_map<Colored::Variable *, std::vector<std::pair<uint32_t, int32_t>>>  varModifierMap;
            arc.expr->getVariables(variables, varPositions, varModifierMap);

            std::unordered_map<const Colored::Color*, std::vector<uint32_t>> constantMap;
            uint32_t index = 0;
            arc.expr->getConstants(constantMap, index);

            if (!variables.empty()) {
                transitionHasVarOutArcs = true;

                auto intervals = arc.expr->getOutputIntervals(&transition.variableIntervals);

                for(auto interval : intervals._ranges){
                    placeFixpoint.constraints.addInterval(interval);    
                }
                                      
            }

            for(auto color: constantMap) {
                std::vector<uint32_t> idVector;
                Reachability::interval_t interval;
                color.first->getTupleId(&idVector);

                if(idVector.size() != placeFixpoint.productSize){
                    //If we are looking at a color wich is only part of a product, we will not add it here
                    // It is added during getting output intervals of arcs
                    continue;
                }

                for(auto id : idVector) {
                    interval.addRange(id, id);
                }

                placeFixpoint.constraints.addInterval(interval);
            }

            
            placeFixpoint.constraints.mergeIntervals();            

            if (!placeFixpoint.inQueue) {
                uint32_t colorsAfter = 0;
                for (auto interval : placeFixpoint.constraints._ranges) {
                    for(auto range : interval._ranges) {
                        colorsAfter += 1 + range._upper - range._lower;
                    }                    
                }
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

    void ColoredPetriNetBuilder::collectVarPlaceRestrictions(Colored::VariableInterval &guardVarInterval, PetriEngine::Colored::ColorFixpoint& colorfixpoint, std::set<uint32_t> varIndexes, std::unordered_map<Colored::Variable *, std::vector<std::pair<uint32_t, int32_t>>> varModifierMap){
        int32_t varModifier = 0;
        for (auto varPair : varModifierMap[guardVarInterval._variable]){
            if(std::abs(varPair.second) > std::abs(varModifier)){
                varModifier = varPair.second;
            }
        }
        std::cout << "Found varmodifier: " << varModifier << std::endl; 

        std::vector<Colored::ColorType *> varColorTypes;
        guardVarInterval._variable->colorType->getColortypes(varColorTypes);
        
        for (uint32_t index : varIndexes) {
            for(auto placeInterval : colorfixpoint.constraints._ranges){
                std::vector<Reachability::interval_t> newIntervals;
                std::vector<Reachability::interval_t> tempIntervals;
                std::vector<Reachability::interval_t> collectedIntervals;
                uint32_t j = 0;
                for(uint32_t i = index; i < index + guardVarInterval._variable->colorType->productSize(); i++){
                    if((int32_t) placeInterval[i]._lower + varModifier < 0){
                        placeInterval[i]._lower = 0;
                        auto underflow = std::abs((int32_t) placeInterval[i]._lower + varModifier);
                        Reachability::range_t newRange = Reachability::range_t(varColorTypes[j]->size()-(1+ underflow), varColorTypes[j]->size()-1);
                        tempIntervals = newIntervals;

                        if(tempIntervals.empty()){
                            Reachability::interval_t newInterval;
                            newInterval.addRange(newRange);
                            tempIntervals.push_back(newInterval);
                        } else {
                            for(auto& interval : tempIntervals){
                                interval.addRange(newRange);
                            }
                        }
                        for (auto interval : tempIntervals){
                            collectedIntervals.push_back(interval);
                        }                                        
                    } else {
                        placeInterval[i]._lower += varModifier;
                    }
                    if(placeInterval[i]._upper + varModifier > varColorTypes[j]->size()-1){
                        placeInterval[i]._upper = varColorTypes[j]->size()-1;
                        auto overflow = placeInterval[i]._upper + varModifier + 1 - varColorTypes[j]->size();
                        Reachability::range_t newRange = Reachability::range_t(0, overflow);
                        tempIntervals = newIntervals;

                        if(tempIntervals.empty()){
                            Reachability::interval_t newInterval;
                            newInterval.addRange(newRange);
                            tempIntervals.push_back(newInterval);
                        } else {
                            for(auto& interval : tempIntervals){
                                interval.addRange(newRange);
                            }
                        }
                        for (auto interval : tempIntervals){
                            collectedIntervals.push_back(interval);
                        }    
                    } else {
                        placeInterval[i]._upper += varModifier;
                    }

                    if(newIntervals.empty()){
                        Reachability::interval_t newInterval;
                        newInterval.addRange(placeInterval[i]);
                        newIntervals.push_back(newInterval);
                    } else {
                        for(auto& interval : newIntervals){
                            interval.addRange(placeInterval[i]);
                        }
                    }

                    for (auto interval : collectedIntervals){
                        newIntervals.push_back(interval);
                    }  
                    
                    j++;
                }
                for (auto newInterval : newIntervals){
                    guardVarInterval._ranges.addInterval(newInterval);
                }                                
            }
        }

        std::cout << "Found interval for var: " << std::endl;
        guardVarInterval.print();
    }

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();
            //for (auto& place : _places) {
            //    unfoldPlace(place);
            //}

            std::cout << "Unfolding" << std::endl;

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

        // std::cout << "Transition " << transition.name << std::endl;
        // for(auto varInterval : transition.variableIntervals){
        //     varInterval.second.print();
        // }

        BindingGenerator gen(transition, _colors);
        size_t i = 0;
        for (auto b : gen) {

            //Print all bindings
            // std::cout << transition.name << std::endl;
            // for (auto test : b){
            //     std::cout << "Binding '" << test.first << "\t" << *test.second << "' in bindingds." << std::endl;
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

    void ColoredPetriNetBuilder::unfoldArc(Colored::Arc& arc, Colored::ExpressionContext::BindingMap& binding, std::string& tName, bool input) {
        Colored::ExpressionContext context {binding, _colors};
        auto ms = arc.expr->eval(context);       

        for (const auto& color : ms) {
            if (color.second == 0) {
                continue;
            }
            PetriEngine::Colored::Place place = _places[arc.place];
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
        _generator->nextBinding();
        if (_generator->_isDone) {
            _generator = nullptr;
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
        _expr = _transition.guard;
        std::set<Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
            //_expr->getPatterns(_patterns, _colorTypes);
        }
        for (auto arc : _transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : _transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
            //arc.expr->getPatterns(_patterns, _colorTypes);
        }
        
        
        for (auto var : variables) {
            if(_transition.variableIntervals[var->name]._ranges._ranges.empty()){
                _noValidBindings = true;
                break;
            }
            auto color = var->colorType->getColor(_transition.variableIntervals[var->name]._ranges.getLowerIds());
            _bindings[var->name] = color;
        }
        
        if (!_noValidBindings && !eval())
            nextBinding();
    }


    bool BindingGenerator::eval() {
        if (_expr == nullptr)
            return true;

        Colored::ExpressionContext context {_bindings, _colorTypes};
        return _expr->eval(context);
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            
            for (auto& _binding : _bindings) {
                auto varInterval = _transition.variableIntervals[_binding.first];
                std::vector<uint32_t> colorIds;
                _binding.second->getTupleId(&colorIds);
                auto nextIntervalBinding = varInterval._ranges.isRangeEnd(colorIds);

                if (nextIntervalBinding.size() == 0){
                    _binding.second = &_binding.second->operator++();
                    break;                    
                } else {
                    _binding.second = _binding.second->getColorType()->getColor(nextIntervalBinding.getLowerIds());
                }
            }
            
            if (isInitial()) {
                _isDone = true;
                break;
            }      
            test = eval();
            /*if(_transition.name == "Start"){
                for (auto& _binding : _bindings){
                    cout << "color " << _binding.second->getColorName() << " for " << _binding.first << " is " << test << endl;
                }                
            }*/         
        }
        
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::currentBinding() {
        return _bindings;
    }

    bool BindingGenerator::isInitial() const {
        for (auto& b : _bindings) {
            std::vector<uint32_t> colorIds;
            b.second->getTupleId(&colorIds);
            if (colorIds != _transition.variableIntervals[b.first]._ranges.getLowerIds()) return false;
        }
        return true;
    }

    // bool BindingGenerator::isFinal() const {
    //     for (auto& b : _bindings) {
    //         if (b.second->getId() != _transition.variableIntervals[b.first].getLower()) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }

    BindingGenerator::Iterator BindingGenerator::begin() {
        if(_noValidBindings){
            return {nullptr};
        }
        return {this};
    }

    BindingGenerator::Iterator BindingGenerator::end() {
        return {nullptr};
    }

}

