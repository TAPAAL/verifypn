#include "PetriEngine/Colored/Patterns.h"
#include "PetriEngine/Colored/Colors.h"
#include "PetriEngine/Colored/Expressions.h"


namespace PetriEngine {
    namespace Colored {
        Pattern::Pattern(PatternType patterntype, const Colored::Expression* expr, std::set<Variable*> variables, ColorType* colorType) 
                : _colorType(colorType), _variables(variables), _expr(expr), _patternType(patterntype){
                    std::cout << patterntype << std::endl;
                }
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
            //std::cout << "{" << _patternType << "," << _expr->toString() << ","<<_colorType->getName() << "}" << std::endl;
            std::cout << "{" << _patternType << ","  << "}" << std::endl;

        }

    }
}