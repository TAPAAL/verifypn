#ifndef GUARD_EXPRESSION_H
#define GUARD_EXPRESSION_H

#include "Binding.h"
#include "../Colored/Expressions.h"
#include "../Colored/Colors.h"
namespace PetriEngine {
    namespace ExplicitColored {
        struct GuardExpression
        {
            GuardExpression(std::shared_ptr<Colored::ColorTypeMap> colorTypeMap, Colored::GuardExpression_ptr guardExpression, std::shared_ptr<std::unordered_map<std::string, Variable_t>> variableMap);
            
            GuardExpression(const GuardExpression&) = default;
            GuardExpression(GuardExpression&&) = default;
            GuardExpression& operator=(const GuardExpression&) = default;
            GuardExpression& operator=(GuardExpression&&) = default;
            
            bool eval(const Binding &binding);
        private:
            std::shared_ptr<Colored::ColorTypeMap> _colorTypeMap;
            Colored::GuardExpression_ptr _guardExpression;
            std::shared_ptr<std::unordered_map<std::string, Variable_t>> _variableMap;
        };
    }
}

#endif