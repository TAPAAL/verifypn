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
            std::vector<std::pair<uint32_t, uint32_t>> constraints;
            bool inQueue;

            bool constainsColor(const Color *color) {
                std::vector<uint32_t> colorIdVector;
                color->getTupleId(&colorIdVector);
                for (uint32_t i = 0; i < colorIdVector.size(); i++) {
                    auto colorId = colorIdVector[i];
                    auto constraintsPair = constraints[i];
                    if (colorId < constraintsPair.first || colorId > constraintsPair.second){
                        return false;
                    }                         
                }

                return true;
            }
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

