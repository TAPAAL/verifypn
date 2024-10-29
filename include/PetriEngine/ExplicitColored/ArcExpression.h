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
            ArcExpression(Colored::ArcExpression_ptr arcExpression, std::shared_ptr<Colored::ColorTypeMap> colorTypeMap, std::shared_ptr<std::unordered_map<std::string, Variable_t>> variableMap);
            
            ArcExpression(const ArcExpression&) = default;
            ArcExpression(ArcExpression&&) = default;
            ArcExpression& operator=(const ArcExpression&) = default;
            ArcExpression& operator=(ArcExpression&&) = default;
            
            [[nodiscard]] CPNMultiSet eval(const Binding &binding) const;
            [[nodiscard]] const std::set<Variable_t>& getVariables() const;
        private:
            std::shared_ptr<Colored::ColorTypeMap> _colorTypeMap;
            std::shared_ptr<std::unordered_map<std::string, Variable_t>> _variableMap;
            Colored::ArcExpression_ptr _arcExpression;
            std::set<Variable_t> _variables;
        };

    }
}

#endif