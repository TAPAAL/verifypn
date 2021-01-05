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

            Colored::ColorFixpoint colorFixpoint;
            colorFixpoint.inQueue = !tokens.empty();

            for (auto colorPair : tokens) {
                colorFixpoint.colors.insert(colorPair.first);
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
            std::cout << "[ ";
            for(auto color : placeColorFixpoint.colors) {
                std::cout << color->getId() << " ";
                                   
            }
            std::cout << "]"<< std::endl; 

            std::cout << std::endl;
        }
        

    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint() {

        auto start = std::chrono::high_resolution_clock::now();

        while(!_placeFixpointQueue.empty()){
            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();
            _placeColorFixpoints[currentPlaceId].inQueue = false;
 
            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];
            // std::cout << "Place " << _places[currentPlaceId].name << std::endl;

            for (uint32_t transitionId : connectedTransitions) {
                
                Colored::Transition& transition = _transitions[transitionId];
                if (transition.considered) continue;
                //maybe this can be avoided with a better implementation
                transition.variableMap.clear();

                // std::cout << "Transtition " << transition.name << std::endl;

                // for(auto& arcInterval : _arcIntervals[transitionId]){
                //     arcInterval.second.resetIntervals();
                // } 
                           
                processInputArcs(transition, currentPlaceId, transitionId);
                              
            }
            
        }

        auto end = std::chrono::high_resolution_clock::now();

        _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        // std::cout << "Total time " << totalinputtime << std::endl;
        // std::cout << "Total time2 " << totalinputtime2 << std::endl;
        
        // printPlaceTable();
        //We should not need to keep colors in places after we have found fixpoint
        _placeColorFixpoints.clear();
    }

    void ColoredPetriNetBuilder::processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId) {
        for (auto arc : transition.input_arcs) {
            //we could also use eval on arcs here
            PetriEngine::Colored::ColorFixpoint& curCFP = _placeColorFixpoints[arc.place];
            // std::set<const Colored::Variable *> vars;
            // std::vector<Colored::ExprVariable> varPositions;
            // arc.expr->getVariables(vars, varPositions);
            
            // std::vector<Colored::ExpressionContext::BindingMap> bindings;

            // for(auto varPosition : varPositions){
            //     if(bindings.empty()){
            //         for(auto color : curCFP.colors){
            //             Colored::ExpressionContext::BindingMap newBinding;
            //             if(varPosition.inTuple){
            //                 newBinding[varPosition.variable] = color->operator[](varPosition.index);
            //             } else {
            //                 newBinding[varPosition.variable] = color;
            //             }
            //             bindings.push_back(newBinding);
            //         } 
            //     } else {
            //         std::vector<Colored::ExpressionContext::BindingMap> tempBindings;
            //         for(auto binding : bindings){
            //             for(auto color : curCFP.colors){
            //                 Colored::ExpressionContext::BindingMap bindingCopy = binding;
            //                 if(varPosition.inTuple){
            //                     bindingCopy[varPosition.variable] = color->operator[](varPosition.index);
            //                 } else {
            //                     bindingCopy[varPosition.variable] = color;
            //                 }
            //                 tempBindings.push_back(bindingCopy);                            
            //             }
            //         }
            //         bindings = std::move(tempBindings);
            //     }
            // }

            // for(auto binding : bindings){
            //     Colored::ExpressionContext context {binding, _colors};
            //     auto res = arc.expr->eval(context);
            // }
            
            
            uint32_t index = 0;
            auto arcColors = arc.expr->findInputColors(curCFP.colors, &index, false);
            index = 0;

            if(!arc.expr->createInputBindings(transition.variableMap, arcColors, &index, false)){
                std::cout << "Failed to get arc intervals" << std::endl;
                return;
            }             
        }

        //handle variables not found on input arcs
        std::set<const Colored::Variable *> vars;
        if(transition.guard != nullptr){
            transition.guard->getVariables(vars);
        }
        for(auto arc : transition.output_arcs){
            arc.expr->getVariables(vars);
        }
        for(auto var : vars){
            if(transition.variableMap.count(var) == 0){
                for(auto color : var->colorType->getColorSet()){
                    transition.variableMap[var].insert(color);
                }                
            }
        }

        std::vector<Colored::ExpressionContext::BindingMap> bindings;
        for(auto varPair : transition.variableMap){
            if(bindings.empty()){                    
                for(auto color : varPair.second){
                    Colored::ExpressionContext::BindingMap newBinding;
                    newBinding[varPair.first] = color;
                    bindings.push_back(newBinding);
                }                    
            } else {
                std::vector<Colored::ExpressionContext::BindingMap> tempBindings;
                for(auto binding : bindings){
                    for(auto color : varPair.second){
                        Colored::ExpressionContext::BindingMap bindingCopy = binding;
                        bindingCopy[varPair.first] = color;
                        tempBindings.push_back(bindingCopy);                            
                    }
                }
                bindings = std::move(tempBindings);
            }                
        }     

        //handle case where there is no variables
        if(transition.variableMap.empty()){
            Colored::ExpressionContext::BindingMap emptyBinding;
            bindings.push_back(emptyBinding);
        }

        if(transition.guard != nullptr) {
            for(auto& binding : bindings){
                Colored::ExpressionContext context {binding, _colors};
                if(transition.guard->eval(context)){
                    _validBindings[transitionId].push_back(std::move(binding));
                }
            }
        } else {
            _validBindings[transitionId] = std::move(bindings);
        }

        for(auto arc : transition.output_arcs){
            Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
            size_t colorsBefore = placeFixpoint.colors.size();
            for(auto binding : _validBindings[transitionId]){
                Colored::ExpressionContext context {binding, _colors};
                for(auto color : arc.expr->eval(context)){
                    placeFixpoint.colors.insert(color.first);
                }
            }
            if(placeFixpoint.colors.size() > colorsBefore){
                _placeFixpointQueue.push_back(arc.place);
                placeFixpoint.inQueue = true;
            }
        }                                         
    }

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();
            //for (auto& place : _places) {
            //    unfoldPlace(place);
            //}

            std::cout << "Unfolding " << std::endl;

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
        size_t i = 0;
        for (auto b : _validBindings[_transitionnames[transition.name]]) {

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
}

