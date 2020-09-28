#include "PetriEngine/Colored/Patterns.h"
#include "PetriEngine/Colored/Colors.h"
#include "PetriEngine/Colored/Expressions.h"


namespace PetriEngine {
    namespace Colored {
        Pattern::Pattern(PatternType patterntype, const Colored::Expression* expr, std::set<Variable*> variables, ColorType* colorType) 
                : _colorType(colorType), _variables(variables), _expr(expr), _patternType(patterntype){}
        bool Pattern::operator== (const Pattern& other) const {
            return _variables == other._variables && _colorType == other._colorType;
        }
        bool Pattern::operator< (const Pattern& other) const {
            for (auto _var : _variables) {
                if (other._variables.find(_var) == other._variables.end()) {
                    return false;
                }
            } 
            return true;
        }

        void Pattern::toString () const {
            std::string variableString = "";
            for(auto variable : _variables){
                variableString.append(variable->name);
            }
            if(_colorType != nullptr){
                std::cout << "{" << _patternType << "," << _expr->toString() << ","<< "{" << variableString << "}, " <<_colorType->getName() << "}" << std::endl;
            } else{
                std::cout << "{" << _patternType << ","  << _expr->toString()<< ","<< "{" << variableString << "}}" << std::endl;
            }
        }
    }
}