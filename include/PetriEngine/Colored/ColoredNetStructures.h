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
#include <assert.h>
#include "Colors.h"
#include "Expressions.h"
#include "Multiset.h"
#include "../TAR/range.h"

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
            Reachability::rangeInterval_t constraints;
            bool inQueue;
            uint32_t productSize;

            bool constainsColor(std::pair<const PetriEngine::Colored::Color *const, std::vector<uint32_t>> constPair) {
                std::unordered_map<uint32_t, bool> contained;
                for(auto interval : constraints._ranges) {
                    for(uint32_t id : constPair.second){
                        
                        if(contained[id] != true){
                            contained[id] = interval[id].contains(constPair.first->getId());
                        }                        
                    }
                }

                for(auto pair : contained){
                    if (!pair.second){
                        return false;
                    }
                }
                return true;
            }
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

