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
            std::set<Colored::Color> placeColors;
            _places.emplace_back(Colored::Place {name, type, tokens});
            _placenames[name] = next;

            //set up place color fix points and initialize queue
            if (!tokens.empty()) {
                _placeFixpointQueue.emplace_back(next);
            }

            for (auto colorPair : tokens) {
                placeColors.insert(*colorPair.first);
            }

            Colored::ColorFixpoint colorFixpoint = {placeColors,!tokens.empty()};

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

            for (auto color : placeColorFixpoint.colors){
                std::cout << color.toString() << "\t";
            }
            std::cout << std::endl;
        }
        

    }

    void ColoredPetriNetBuilder::computePlaceColorFixpoint() {
        
        while(!_placeFixpointQueue.empty()){
            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();

            PetriEngine::Colored::ColorFixpoint& colorfixpoint = _placeColorFixpoints[currentPlaceId];

            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];
            
            for (uint32_t transitionId : connectedTransitions) {
                
                Colored::Transition& transition = _transitions[transitionId];

                bool transitionActivated = true;

                for (auto& arc : transition.input_arcs) {
                    if (arc.place == currentPlaceId) {
                        std::set<Colored::Variable *> variables;
                        arc.expr.get()->getVariables(variables);

                        if (variables.empty()) {
                            // check what colors are on the arcs againts the ones in this place
                            
                        } else {
                            auto bindingMap = _bindings.find(transitionId);

                            if ( bindingMap == _bindings.end()) {
                                BindingGenerator gen {transition, _colors};
                                _bindings.emplace(std::make_pair(transitionId, gen));

                                for (auto binding : gen) {

                                    Colored::ExpressionContext context {binding, _colors};           

                                    Colored::Multiset ms = arc.expr->eval(context);

                                    std::set<Colored::Color> colors;
                                    //extract the color vector from the multiset
                                    for (auto colorPair : ms) {
                                        colors.insert(*colorPair.first);
                                    }
                                    
                                    if (std::includes(colorfixpoint.colors.begin(), colorfixpoint.colors.end(), 
                                    colors.begin(), colors.end())) {
                                        arc.activatable = true;
                                    } else {
                                        transitionActivated = false;
                                        break;
                                    }
                                            
                                }                                 
                            } else for (auto binding : bindingMap->second) {

                                Colored::ExpressionContext context {binding, _colors};

                                
                                bool transitionActivated = true;

                                if (arc.place == currentPlaceId) {

                                    Colored::Multiset ms = arc.expr->eval(context);

                                    std::set<Colored::Color> colors;
                                    //extract the color vector from the multiset
                                    for (auto colorPair : ms) {
                                        colors.insert(*colorPair.first);
                                    }
                                    
                                    if (std::includes(colorfixpoint.colors.begin(), colorfixpoint.colors.end(), 
                                    colors.begin(), colors.end())) {
                                        arc.activatable = true;
                                    } else {
                                        transitionActivated = false;
                                        break;
                                    }
                                    
                                } else if (!arc.activatable) {
                                    transitionActivated = false;
                                }                                
                            }
                        }
                    } else if (!arc.activatable) {
                        transitionActivated = false;
                    }                
                }

                if (transitionActivated) {
                    for (auto& arc : transition.output_arcs) {
                        Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
                        if (placeFixpoint.inQueue) break;
                        
                        //Colored::Multiset ms = arc.expr->eval(context);
                        //If variable, use colortsum += t.input_arcs.size();ype otherwise use color

                        auto colorsBefore = placeFixpoint.colors.size();

                        std::set<Colored::Color> colors;
                        //extract the color vector from the multiset
                        // for (auto colorPair : ms) {
                        //     colors.insert(*colorPair.first);
                        // }
                        placeFixpoint.colors.insert(colors.begin(), colors.end());
                        
                        if (placeFixpoint.colors.size() > colorsBefore){
                            _placeFixpointQueue.push_back(arc.place);
                            placeFixpoint.inQueue = true;
                        }                           
                    }
                } 
                
                

                //update the colortype of variables such that bindings generate only valid vlaues

                //split arcs into input and output arcs

                //find out if transition is enabled 
                // by getting all places leading into the transition
                // and checking if their colorsets can satisfy what the arc expressions want
                // then consider guards
                //update colors for place recieving outputs from transition 
                //(possibly based on restrictions of variables)

            }


        }
    }

    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (_stripped) assert(false);
        if (_isColored && !_unfolded) {
            auto start = std::chrono::high_resolution_clock::now();
            for (auto& place : _places) {
                unfoldPlace(place);
            }

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
        for (size_t i = 0; i < place.type->size(); ++i) {
            std::string name = place.name + ";" + std::to_string(i);
            const Colored::Color* color = &place.type->operator[](i);
            _ptBuilder.addPlace(name, place.marking[color], 0.0, 0.0);
            _ptplacenames[place.name][color->getId()] = std::move(name);
            ++_nptplaces;
        }
    }

    void ColoredPetriNetBuilder::unfoldTransition(Colored::Transition& transition) {
        BindingGenerator gen(transition, _colors);
        size_t i = 0;
        for (auto& b : gen) {
            std::string name = transition.name + ";" + std::to_string(i++);
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
            const std::string& pName = _ptplacenames[_places[arc.place].name][color.first->getId()];
            if (input) {
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

