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
            addTransition(transition,0.0,0.0);
        }
        if(_placenames.count(place) == 0)
        {
            addPlace(place,0,0,0);
        }
        uint32_t p = _placenames[place];
        uint32_t t = _transitionnames[transition];
        
        assert(t < _transitions.size());
        assert(p < _places.size());
        
        Colored::Arc arc;
        arc.place = p;
        arc.transition = p;
        arc.expr = expr;
        arc.input = input;
        _arcs.push_back(arc);
    }
    
    void ColoredPetriNetBuilder::addColorType(const std::string& id, Colored::ColorType* type) {
        _colors[id] = type;
    }
    
    void ColoredPetriNetBuilder::sort() {
        
    }
    
    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        if (isColored) {
            for (auto place : _places) {
                unfoldPlace(place);
            }

            for (auto transition : _transitions) {
                unfoldTransition(transition);
            }

            for (auto arc : _arcs) {
                unfoldArc(arc);
            }
        }

        return _ptBuilder;
    }
    
    void ColoredPetriNetBuilder::unfoldPlace(Colored::Place& place) {
        for (auto c : *place.type) {
            _ptBuilder.addPlace(place.name + ";" + c.toString(), place.marking[c]);
        }
    }
    
    void ColoredPetriNetBuilder::unfoldTransition(Colored::Transition& transition) {
        
        for (auto b : transition) {

        }

    }
}

