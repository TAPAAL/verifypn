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

namespace PetriEngine {
    namespace Colored {
        
        struct Arc {
            
        };
        
        struct Transition {
            const char* name;
            
        };
        
        struct Place {
            const char* name;
            std::multiset<Color> marking;
        };
    }
}

#endif /* COLOREDNETSTRUCTURES_H */

