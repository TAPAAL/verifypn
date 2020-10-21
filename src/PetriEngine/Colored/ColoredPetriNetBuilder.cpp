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

            std::vector<std::pair<uint32_t, uint32_t>> placeConstraints;
            uint32_t index = 0;

            for (auto colorPair : tokens) {
                colorPair.first->getColorConstraints(&placeConstraints, &index);
                index = 0;
            }
            Colored::ColorFixpoint colorFixpoint = {placeConstraints, !tokens.empty()};

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
            
            uint32_t index = 0;

            // for (auto color : *place.type) {
            //     std::cout << "Color " << color.toString() << " has index " << index << std::endl;
            //     index++;
            // }
            for(auto fixpointPair : placeColorFixpoint.constraints) {
                    std::cout << fixpointPair.first << "-" << fixpointPair.second << std::endl;
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


                
                processInputArcs(transition, currentPlaceId, transitionActivated);
                
                if (transitionActivated) {
                    processOutputArcs(transition);
                }              
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        //printPlaceTable();
        //We should not need to keep colors in places after we have found fixpoint
        _placeColorFixpoints.clear();
    }

    void ColoredPetriNetBuilder::processInputArcs(Colored::Transition& transition, uint32_t currentPlaceId, bool &transitionActivated) {

        PetriEngine::Colored::ColorFixpoint& colorfixpoint = _placeColorFixpoints[currentPlaceId];
        colorfixpoint.inQueue = false;

        bool curPlaceArcProcessed = false;
        
        std::set<Colored::Variable *> transitionVars;
        std::unordered_map<string, std::set<uint32_t>> varGuardPositions;
        if (transition.guard != nullptr) {
            transition.guard->getVariables(transitionVars, varGuardPositions);
        }
        
        for (auto& arc : transition.input_arcs) {

            //std::cout << "Arc " << arc.place << "->" << arc.transition << " with expr: " << arc.expr->toString() << std::endl;

            //Once we have considered the arc connecting to the current place 
            //we can break if transitions are not activated
            if (!transitionActivated && curPlaceArcProcessed) {
                break;
            }

            if (arc.place == currentPlaceId) {
                bool succes = true;
                std::set<Colored::Variable *> variables;
                std::unordered_map<string, std::set<uint32_t>> varPositions;
                arc.expr->getVariables(variables, varPositions);

                //std::cout << "Found " << variables.size() << " vars out here" << std::endl;

                if (!variables.empty()) {                            

                    for (auto var : variables) {
                        //std::cout << "Found " << var->name << " for transition " << transition.name << std::endl;
                        
                        auto varIndexes = varPositions[var->name];
                        Colored::VariableInterval varInterval;

                        // find way to not always compute the guard
                        Colored::VariableInterval guardVarInterval = {var, 0, (uint32_t) var->colorType->size()-1};

                        if (transition.guard != nullptr && transitionVars.find(var) != transitionVars.end()) {
                            transition.guard->restrictVar(&guardVarInterval);
                        } 

                        if (transition.variableIntervals.count(var->name) == 0) {
                            transition.variableIntervals[var->name] = guardVarInterval;
                        } 

                        varInterval = transition.variableIntervals[var->name];                                                        

                        for (uint32_t index : varIndexes) {
                            if (varInterval.interval_lower < colorfixpoint.constraints[index].first) {
                                varInterval.interval_lower = colorfixpoint.constraints[index].first;
                            } else if (colorfixpoint.constraints[index].first >= guardVarInterval.interval_lower) {
                                varInterval.interval_lower = colorfixpoint.constraints[index].first;
                            }

                            if (varInterval.interval_upper > colorfixpoint.constraints[index].second) {
                                varInterval.interval_upper = colorfixpoint.constraints[index].second;
                            } else if (colorfixpoint.constraints[index].second <= guardVarInterval.interval_upper){
                                varInterval.interval_upper = colorfixpoint.constraints[index].second;
                            }

                            if (varInterval.interval_upper < colorfixpoint.constraints[index].first || 
                            varInterval.interval_lower > colorfixpoint.constraints[index].second) {
                                //If the arc connected to the place under consideration cannot be activated,
                                //then there is no reason to keep checking
                                transitionActivated = false;
                                succes = false;
                                break;
                            }
                        }
                        transition.variableIntervals[var->name] = varInterval;                              
                    } 
                }                           

                std::set<const Colored::Color*> colors = arc.expr->getConstants();
                
                    for (auto color : colors) {
                    if(!colorfixpoint.constainsColor(color)) {
                        //If the arc connected to the place under consideration cannot be activated,
                        //then there is no reason to keep checking
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

            //used to check if colors are added to the place. The total distance between upper and
            //lower bounds should grow when more colors are added and as we cannot remove colors this
            //can be checked by summing the differences
            uint32_t colorsBefore = 0;
            for (auto constraint : placeFixpoint.constraints) {
                colorsBefore += constraint.second - constraint.first;
            }
                
            std::set<Colored::Variable *> variables;
            std::unordered_map<string, std::set<uint32_t>> varPositions;
            arc.expr->getVariables(variables, varPositions);

            std::set<const Colored::Color*> colors = arc.expr->getConstants();

            if (!variables.empty()) {
                transitionHasVarOutArcs = true;

                auto intervals = arc.expr->getOutputIntervals(&transition.variableIntervals);

                for(uint32_t i = 0; i < intervals.size(); i++){
                    auto intervalPair = intervals[i];
                    if (i >= placeFixpoint.constraints.size()) {
                        placeFixpoint.constraints.emplace_back(intervalPair);
                    } else {
                        auto fixpointPair = &placeFixpoint.constraints[i];
                        if (intervalPair.first < fixpointPair->first) {
                            fixpointPair->first = intervalPair.first;
                        }
                        if(intervalPair.second > fixpointPair->second) {
                            fixpointPair->second = intervalPair.second;
                        }
                    }
                }                          
            }

            for(auto color: colors) {
                std::vector<uint32_t> idVector;
                color->getTupleId(&idVector);
                for (uint32_t i = 0; i < idVector.size(); i++) {
                    uint32_t id = idVector[i];
                    if (i >= placeFixpoint.constraints.size()) {
                        placeFixpoint.constraints.emplace_back(std::make_pair(id,id));
                    } else {
                        auto intervalPair = &placeFixpoint.constraints[i];
                        if ( id < intervalPair->first) {
                            intervalPair->first = id;
                        }
                        if (id > intervalPair->second) {
                            intervalPair->second = id;
                        }
                    }
                }
            }

            if (!placeFixpoint.inQueue) {
                uint32_t colorsAfter = 0;
                for (auto constraint : placeFixpoint.constraints) {
                    colorsAfter += constraint.second - constraint.first;
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

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();
            //for (auto& place : _places) {
            //    unfoldPlace(place);
            //}

            for (auto& transition : _transitions) {
                unfoldTransition(transition);
            }

            _unfolded = true;
            auto end = std::chrono::high_resolution_clock::now();
            _time = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
        }

        return _ptBuilder;
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
        BindingGenerator gen(transition, _colors);
        size_t i = 0;
        for (auto b : gen) {

            //Print all bindings
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
            auto color = &var->colorType->operator[](_transition.variableIntervals[var->name].interval_lower);
            _bindings[var->name] = color;
        }
        
        if (!eval())
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

                if (_binding.second->getId() == varInterval.interval_upper){
                    _binding.second = &_binding.second->getColorType()->operator[](varInterval.interval_lower);
                } else {
                    _binding.second = &_binding.second->operator++();
                    break;
                }
            }

            if (isFinal()) {
                _isDone = true;
                break;
            }                

            test = eval();
        }
        return _bindings;
    }

    Colored::ExpressionContext::BindingMap& BindingGenerator::currentBinding() {
        return _bindings;
    }

    bool BindingGenerator::isInitial() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != 0) return false;
        }
        return true;
    }

    bool BindingGenerator::isFinal() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != _transition.variableIntervals[b.first].interval_upper) {
                return false;
            }
        }
        return true;
    }

    BindingGenerator::Iterator BindingGenerator::begin() {
        return {this};
    }

    BindingGenerator::Iterator BindingGenerator::end() {
        return {nullptr};
    }

}

