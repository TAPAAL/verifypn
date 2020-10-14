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
            ArcExpression_ptr expr;
            bool activatable;
            bool input;
        };
        
        struct Transition {
            std::string name;
            GuardExpression_ptr guard;
            std::vector<Arc> input_arcs;
            std::vector<Arc> output_arcs;
            std::unordered_map<std::string, Colored::VariableInterval> variableIntervals;
            bool considered;
        };
        
        struct Place {
            std::string name;
            ColorType* type;
            Multiset marking;
        };

        struct ColorFixpoint {
            uint32_t interval_lower;
            uint32_t interval_upper;
            bool inQueue;

            bool constainsColor(uint32_t colorId) {
                return interval_lower <= colorId && colorId <= interval_upper;
            }
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

