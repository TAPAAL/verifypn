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
        
    }
    
    void ColoredPetriNetBuilder::addPlace(const std::string& name, Colored::Multiset tokens, double x, double y) {
        
    }
    
    void ColoredPetriNetBuilder::addTransition(const std::string& name, double x, double y) {
        
    }
    
    void ColoredPetriNetBuilder::addTransition(const std::string& name, Colored::GuardExpression* guard, double x, double y) {
        
    }
    
    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, bool inhibitor, int) {
        
    }
    
    void ColoredPetriNetBuilder::addInputArc(const std::string& place, const std::string& transition, Colored::ArcExpression* expr) {
        
    }
    
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, int weight) {
        
    }
    
    void ColoredPetriNetBuilder::addOutputArc(const std::string& transition, const std::string& place, Colored::ArcExpression* expr) {
        
    }
    
    void ColoredPetriNetBuilder::sort() {
        
    }
    
    PetriNetBuilder& ColoredPetriNetBuilder::unfold() {
        return _ptBuilder;
    }
}

