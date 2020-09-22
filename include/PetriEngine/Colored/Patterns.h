/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Patterns.h
 * Author: os
 *
 * Created on February 19, 2018, 7:00 PM
 */

#ifndef PATTERNS_H
#define PATTERNS_H

#include <vector>
#include <unordered_map>
#include <optional>

#include "ColoredNetStructures.h"

namespace PetriEngine {
    namespace Colored {
        enum PatternType {
            ArcExp = 1,
            Guard = 2,
        };

        struct Pattern {
                PatternType patternType;
                std::shared_ptr<Colored::Expression> expr;
                std::set<Variable*> variables; //This could instead be Variable* (name, colortype), but then the colortype is duplicated
                ColorType* colorType;

                bool operator== (const Pattern& other) const {
                    return variables == other.variables && colorType == other.colorType;
                }
                bool operator< (const Pattern& other) const {
                    for (auto _var : variables) {
                        if (other.variables.find(_var) == other.variables.end()) {
                            return false;
                        }
                    } 
                    return true;
                }

                void toString () const {
                    std::cout << "{" << patternType << "," << expr->toString() << ","<<colorType->getName() << "}" << std::endl;
                }
        };

        typedef std::set<const Colored::Pattern*> PatternSet;

    }
}



#endif /* PATTERNS_H */
