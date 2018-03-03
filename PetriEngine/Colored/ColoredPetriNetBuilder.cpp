/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ColoredPetriNetBuilder.cpp
 * Author: Klostergaard
 * 
 * Created on 17. februar 2018, 16:25
 */

#include "ColoredPetriNetBuilder.h"

namespace PetriEngine {
    ColoredPetriNetBuilder::ColoredPetriNetBuilder() {
    }

    ColoredPetriNetBuilder::ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig) {
    }

    ColoredPetriNetBuilder::~ColoredPetriNetBuilder() {
    }
    
    void ColoredPetriNetBuilder::addPlace(const std::string& name, int tokens, double x, double y) {
        if (!isColored) {
            _ptBuilder.addPlace(name, tokens, x, y);
        }
    }
    
    void ColoredPetriNetBuilder::addPlace(const std::string& name, Colored::ColorType* type, Colored::Multiset tokens, double x, double y) {
        if(_placenames.count(name) == 0)
        {
            uint32_t next = _placenames.size();
            _places.push_back(Colored::Place {name, type, tokens});
            _placenames[name] = next;
        }
    }
    
    void ColoredPetriNetBuilder::addTransition(const std::string& name, double x, double y) {
        if (!isColored) {
            _ptBuilder.addTransition(name, x, y);
        }
    }
    
    void ColoredPetriNetBuilder::addTransition(const std::string& name, Colored::GuardExpression* guard, double x, double y) {
        if(_transitionnames.count(name) == 0)
        {
            uint32_t next = _transitionnames.size();
            _transitions.push_back(Colored::Transition {name, guard});
            _transitionnames[name] = next;
        }
    }
    
    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, int weight) {
        if (!isColored) {
            _ptBuilder.addInputArc(place, transition, inhibitor, weight);
        }
    }
    
    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, Colored::ArcExpression* expr) {
        addArc(place, transition, expr, true);
    }
    
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, int weight) {
        if (!isColored) {
            _ptBuilder.addOutputArc(transition, place, weight);
        }
    }
    
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, Colored::ArcExpression* expr) {
        addArc(place, transition, expr, false);
    }
    
    void ColoredPetriNetBuilder::addArc(const std::string& place, const std::string& transition, Colored::ArcExpression* expr, bool input) {
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
        
        //std::set<Colored::Variable*> variables;
        
        Colored::Arc arc;
        arc.place = p;
        arc.transition = t;
        assert(expr != nullptr);
        arc.expr = expr;
        arc.input = input;
        _transitions[t].arcs.push_back(_arcs.size());
        _arcs.push_back(arc);
//        assert(_transitions[t].arcs.back() == &_arcs.back());
    }
    
    void ColoredPetriNetBuilder::addColorType(const std::string& id, Colored::ColorType* type) {
        _colors[id] = type;
    }
    
    void ColoredPetriNetBuilder::sort() {
        
    }
    
    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (isColored) {
            for (auto& place : _places) {
                unfoldPlace(place);
            }

            for (auto& transition : _transitions) {
                unfoldTransition(transition);
            }

            for (auto& arc : _arcs) {
                unfoldArc(arc);
            }
        }

        return _ptBuilder;
    }
    
    void ColoredPetriNetBuilder::unfoldPlace(Colored::Place& place) {
        for (auto c : *place.type) {
            std::string name = place.name + ";" + c.toString();
            _ptBuilder.addPlace(name, place.marking[&c], 0.0, 0.0);
            _ptplacenames[place.name][&c] = name;
        }
    }
    
    void ColoredPetriNetBuilder::unfoldTransition(Colored::Transition& transition) {
        std::cout << transition.name << std::endl;
        BindingGenerator gen(transition, _arcs, _colors);
        for (auto b : gen) {
            size_t i = transition.bindings.size();
            std::unordered_map<std::string, const Colored::Color*> binding;
            for (auto elem : b) {
                binding[elem.var->name] = elem.color;
            }
            transition.bindings.push_back(binding);
            std::string name = transition.name + ";" + std::to_string(i);
            _ptBuilder.addTransition(name, 0.0, 0.0);
            _pttransitionnames[transition.name].push_back(name);
        }
    }
    
    void ColoredPetriNetBuilder::unfoldArc(Colored::Arc& arc) {
        Colored::Transition& transition = _transitions[arc.transition];
        for (size_t i = 0; i < transition.bindings.size(); ++i) {
            Colored::ExpressionContext context {transition.bindings[i], _colors};
            Colored::Multiset ms = arc.expr->eval(context);
            for (auto color : ms) {
                if (color.second == 0)
                    continue;
                std::string pName = _ptplacenames[_places[arc.place].name][color.first];
                std::string tName = _pttransitionnames[transition.name][i];
                if (arc.input) {
                    _ptBuilder.addInputArc(pName, tName, false, color.second);
                } else {
                    _ptBuilder.addOutputArc(tName, pName, color.second);
                }
            }
        }
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
    
    std::vector<Colored::Binding> BindingGenerator::Iterator::operator++(int) {
        auto prev = _generator->currentBinding();
        ++*this;
        return prev;
    }
    
    std::vector<Colored::Binding>& BindingGenerator::Iterator::operator*() {
        return _generator->currentBinding();
    }
    
    BindingGenerator::BindingGenerator(Colored::Transition& transition,
            const std::vector<Colored::Arc>& arcs,
            ColoredPetriNetBuilder::ColorTypeMap& colorTypes)
        : _colorTypes(colorTypes)
    {
        _expr = transition.guard;
        std::set<Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (auto ai : transition.arcs) {
            auto& arc = arcs[ai];
            //arc->expr->expressionType();
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (auto var : variables) {
            _bindings.push_back(Colored::Binding {var, &(*var->colorType)[0]});
        }
        
        if (!eval())
            nextBinding();
    }
    
    bool BindingGenerator::eval() {
        if (_expr == nullptr)
            return true;
        
        std::unordered_map<std::string, const Colored::Color*> binding;
        for (auto elem : _bindings) {
            binding[elem.var->name] = elem.color;
        }
        Colored::ExpressionContext context {binding, _colorTypes};
        return _expr->eval(context);
    }
    
    std::vector<Colored::Binding>& BindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            for (size_t i = 0; i < _bindings.size(); ++i) {
                const Colored::Color* next = ++_bindings[i].color;
                if (next->getId() != 0) {
                    break;
                }
            }
            
            test = eval();
            
            if (isInitial())
                break;
        }
        return _bindings;
    }
    
    std::vector<Colored::Binding>& BindingGenerator::currentBinding() {
        return _bindings;
    }
    
    bool BindingGenerator::isInitial() const {
        for (auto b : _bindings) {
            if (b.color->getId() != 0) return false;
        }
        return true;
    }
    
    BindingGenerator::Iterator BindingGenerator::begin() {
        return Iterator(this);
    }
    
    BindingGenerator::Iterator BindingGenerator::end() {
        return Iterator(nullptr);
    }
}

