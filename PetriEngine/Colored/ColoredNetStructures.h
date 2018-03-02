/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ColoredNetStructures.h
 * Author: Klostergaard
 *
 * Created on 17. februar 2018, 17:07
 */

#ifndef COLOREDNETSTRUCTURES_H
#define COLOREDNETSTRUCTURES_H

#include <vector>
#include <set>
#include "Colors.h"
#include "Expressions.h"
#include "Multiset.h"

namespace PetriEngine {
    namespace Colored {
        
        struct Arc {
            uint32_t place;
            uint32_t transition;
            ArcExpression* expr;
            bool input;
        };
        
        struct Transition {
            std::string name;
            GuardExpression* guard;
            std::vector<std::unordered_map<std::string,const Color*>> bindings;
            std::vector<size_t> arcs;
        };
        
        struct Place {
            std::string name;
            ColorType* type;
            Multiset marking;
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

