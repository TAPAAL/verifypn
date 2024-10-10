#ifndef GUARD_EXPRESSION_H
#define GUARD_EXPRESSION_H

#include "Binding.h"
#include "../Colored/Expressions.h"
#include "../Colored/Colors.h"
namespace PetriEngine {
    namespace ExplicitColored {
        struct GuardExpression
        {
            GuardExpression(std::shared_ptr<Colored::ColorTypeMap> colorTypeMap, Colored::GuardExpression_ptr guardExpression);
            
            GuardExpression(const GuardExpression&) = default;
            GuardExpression(GuardExpression&&) = default;
            GuardExpression& operator=(const GuardExpression&) = default;
            GuardExpression& operator=(GuardExpression&&) = default;
            
            bool eval(const Binding &binding);
        private:
            std::shared_ptr<Colored::ColorTypeMap> _colorTypeMap;
            Colored::GuardExpression_ptr _guardExpression;
        };
    }
}

#endif