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

#include <set>


namespace PetriEngine {
    namespace Colored {
        class Expression;
        class Variable;
        class ColorType;
        enum PatternType {
            Constant = 0,
            Var = 1,
            Guard = 2,
            Tuple = 3,
        };

        class Pattern {
            private:
                PatternType _patternType;
                //std::shared_ptr<Colored::Expression> expr;
                const Colored::Expression* _expr;
                std::set<Variable*> _variables; //This could instead be Variable* (name, colortype), but then the colortype is duplicated
                ColorType* _colorType;
            public:
                Pattern(PatternType patterntype, const Colored::Expression* expr, std::set<Variable*> variables, ColorType* colorType);
                bool operator== (const Pattern& other) const;
                bool operator< (const Pattern& other) const;
                
                void toString () const;
        };

        typedef std::set<Colored::Pattern> PatternSet;

    }
}



#endif /* PATTERNS_H */
