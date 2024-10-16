#ifndef ARC_EXPRESSION_H
#define ARC_EXPRESSION_H
#include "CPNMultiSet.h"
#include "Binding.h"
#include <memory>
#include "../Colored/Expressions.h"
namespace PetriEngine {
    namespace ExplicitColored {
        struct ArcExpression
        {
            ArcExpression(Colored::ArcExpression_ptr arcExpression, std::shared_ptr<Colored::ColorTypeMap> colorTypeMap);
            
            ArcExpression(const ArcExpression&) = default;
            ArcExpression(ArcExpression&&) = default;
            ArcExpression& operator=(const ArcExpression&) = default;
            ArcExpression& operator=(ArcExpression&&) = default;
            
            CPNMultiSet eval(const Binding &binding);
        private:
            std::shared_ptr<Colored::ColorTypeMap> _colorTypeMap;
            std::shared_ptr<std::unordered_map<std::string, Variable_t>> _variableMap;
            Colored::ArcExpression_ptr _arcExpression;
        };

    }
}

#endif