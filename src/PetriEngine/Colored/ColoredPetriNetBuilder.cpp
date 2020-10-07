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
            std::set<const Colored::Color *> placeColors;
            _places.emplace_back(Colored::Place {name, type, tokens});
            _placenames[name] = next;

            //set up place color fix points and initialize queue
            if (!tokens.empty()) {
                _placeFixpointQueue.emplace_back(next);
            }

            uint32_t lower = UINT32_MAX, upper = 0;

            for (auto colorPair : tokens) {
                auto id = colorPair.first->getId();
                if (id < lower) {
                    lower = id;
                } 
                if (id > upper) {
                    upper = id;
                }
                placeColors.insert(colorPair.first);
            }
            Colored::ColorFixpoint colorFixpoint = {lower, upper,!tokens.empty()};

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
            std::cout << "Place: " << place.name << " in queue: " << placeColorFixpoint.inQueue << std::endl;

            for (uint32_t i = placeColorFixpoint.interval_lower; i <= placeColorFixpoint.interval_upper; i++){
                std::cout << place.type->operator[](i).getId() << "\t";
            }
            std::cout << std::endl;
        }
        

    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint() {
        
        while(!_placeFixpointQueue.empty()){
            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();

            PetriEngine::Colored::ColorFixpoint& colorfixpoint = _placeColorFixpoints[currentPlaceId];
            colorfixpoint.inQueue = false;

            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];

            bool hasVarArcs = false;
            
            for (uint32_t transitionId : connectedTransitions) {
                
                Colored::Transition& transition = _transitions[transitionId];
                if (transition.considered) break;

                bool transitionActivated = true, curPlaceArcProcessed = false;
                bool transitionHasVarOutArcs = false;

                for (auto& arc : transition.input_arcs) {
                    //Once we have considered the arc connecting to the current place 
                    //we can break if transitions are not activated
                    if (!transitionActivated && curPlaceArcProcessed) break;

                    if (arc.place == currentPlaceId) {
                        std::set<Colored::Variable *> variables;
                        arc.expr.get()->getVariables(variables);

                        if (variables.empty()) {
                            // check what colors are on the arcs againts the ones in this place
                            auto colors = arc.expr->getConstants();
                            
                            for (auto color : colors) {
                                if(!colorfixpoint.constainsColor(color->getId())) {
                                    //If the arc connected to the place under consideration cannot be activated,
                                    //then there is no reason to keep checking
                                    transitionActivated = false;
                                    break;
                                } 
                            }
                            arc.activatable = true;
                            
                        } else {

                            for (auto var : variables) {
                                if (transition.guard != nullptr) {
                                    transition.guard->restrictVar(var);
                                }
                                
                                if (var->interval_lower < colorfixpoint.interval_lower) {
                                    var->interval_lower = colorfixpoint.interval_lower;
                                }
                                if (var->interval_upper > colorfixpoint.interval_upper) {
                                    var->interval_upper = colorfixpoint.interval_upper;
                                }

                                if (var->interval_upper < colorfixpoint.interval_lower || 
                                    var->interval_lower > colorfixpoint.interval_upper) {
                                    
                                    //If the arc connected to the place under consideration cannot be activated,
                                    //then there is no reason to keep checking
                                    transitionActivated = false;
                                    break;
                                }                                
                            }                            

                            std::set<const Colored::Color*> colors = arc.expr->getConstants();
                            
                              for (auto color : colors) {
                                if(!colorfixpoint.constainsColor(color->getId())) {
                                    //If the arc connected to the place under consideration cannot be activated,
                                    //then there is no reason to keep checking
                                    transitionActivated = false;
                                    break;
                                } 
                            }
                            arc.activatable = true;                                    
                        }
                        curPlaceArcProcessed = true;
                    } else if (!arc.activatable) {
                        transitionActivated = false;
                    }                
                }

                if (transitionActivated) {

                    for (auto& arc : transition.output_arcs) {
                        Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
                        uint32_t colorsBefore = placeFixpoint.interval_upper - placeFixpoint.interval_lower;
                        std::set<Colored::Variable *> variables;
                        arc.expr->getVariables(variables);

                        

                        if (variables.empty()) {
                            auto colors = arc.expr->getConstants();
                            for(auto color: colors) {
                                auto id = color->getId();
                                if (id < placeFixpoint.interval_lower) {
                                    placeFixpoint.interval_lower = id;
                                } else if(id > placeFixpoint.interval_upper) {
                                    placeFixpoint.interval_upper = id;
                                }
                            }

                        } else {
                            transitionHasVarOutArcs = true;
                            std::set<const Colored::Color*> colors = arc.expr->getConstants();

                            for (auto var : variables) {
                                if (var->interval_lower < placeFixpoint.interval_lower) {
                                    placeFixpoint.interval_lower = var->interval_lower;
                                }
                                if(var->interval_upper > placeFixpoint.interval_upper) {
                                    placeFixpoint.interval_upper = var->interval_upper;
                                }
                            }

                            for(auto color: colors) {
                                auto id = color->getId();
                                if (id < placeFixpoint.interval_lower) {
                                    placeFixpoint.interval_lower = id;
                                } else if(id > placeFixpoint.interval_upper) {
                                    placeFixpoint.interval_upper = id;
                                }
                            }
                        }

                        if (!placeFixpoint.inQueue && placeFixpoint.interval_upper - placeFixpoint.interval_lower > colorsBefore){
                            _placeFixpointQueue.push_back(arc.place);
                            placeFixpoint.inQueue = true;
                        }                     
                    }

                    //If there are no variables among the out arcs of a transition 
                    // and it has been activated, there is no reason to cosider it again
                    if(!transitionHasVarOutArcs) {
                        transition.considered = true;
                    }

                }              
            }

            //If the arc from the place does not use variables, we don't need to reconsider it
            //If the same transition had an arc back to the current place, 
            //we will end up reconsidering the place anyway (even though it should not be needed)
            if(!hasVarArcs) colorfixpoint.inQueue = true;
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
        auto placeId = _placenames[place.name];
        auto placeFixpoint = _placeColorFixpoints[placeId];
        for (size_t i = placeFixpoint.interval_lower; i <= placeFixpoint.interval_upper; ++i) {
            std::string name = place.name + "_" + std::to_string(i);
            const Colored::Color* color = &place.type->operator[](i);
            _ptBuilder.addPlace(name, place.marking[color], 0.0, 0.0);
            _ptplacenames[place.name][color->getId()] = std::move(name);
            ++_nptplaces;
        }
    }

    void ColoredPetriNetBuilder::unfoldTransition(Colored::Transition& transition) {
        BindingGenerator gen(transition, _colors);
        size_t i = 0;
        for (auto b : gen) {
            /*
            //Print all bindings
            for (auto test : b){
                std::cout << "Binding '" << test.first << "\t" << *test.second << "' in bindingds." << std::endl;
            }
            std::cout << std::endl;
            */
            
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
        if (_generator->isInitial()) _generator = nullptr;
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
        : _colorTypes(colorTypes)
    {
        _expr = transition.guard;
        std::set<Colored::Variable*> variables;
        //std::set<Colored::Variable*> guardVariables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
            /*_expr->getVariables(guardVariables);
            const Colored::Pattern guardPattern = {
                Colored::PatternType::Guard,
                _expr,
                guardVariables,
                nullptr,
            };
            _patterns.insert(&guardPattern);*/
            _expr->getPatterns(_patterns, _colorTypes);
        }
        for (auto arc : transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto arc : transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
            /*arc.expr->getVariables(arcVariables);
            Colored::ColorType* ctype = places[arc.place].type;
            const Colored::Pattern arcPattern {
                Colored::PatternType::ArcExp,
                arc.expr,
                arcVariables,
                ctype,
            };*/
            arc.expr->getPatterns(_patterns, _colorTypes);
        }
        

        for (auto var : variables) {
            _bindings[var->name] = &var->colorType->operator[](0);
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

    Colored::ExpressionContext::BindingMap& BindingGenerator::currentBinding() {
        return _bindings;
    }

    bool BindingGenerator::isInitial() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != 0) return false;
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

