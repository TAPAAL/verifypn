#ifndef ARC_EXPRESSION_H
#define ARC_EXPRESSION_H
#include "CPNMultiSet.h"
#include "Binding.h"
#include <memory>

#include "CompiledArcExpression.hpp"
#include "../Colored/Expressions.h"

namespace PetriEngine {
    namespace ExplicitColored {
        struct ArcExpression
        {
            ArcExpression(
                const Colored::ArcExpression_ptr& arcExpression,
                const Colored::ColorTypeMap& colorTypeMap,
                const std::unordered_map<std::string, Variable_t>& variableMap
            );
            
            ArcExpression(const ArcExpression&) = default;
            ArcExpression(ArcExpression&&) = default;
            ArcExpression& operator=(const ArcExpression&) = default;
            ArcExpression& operator=(ArcExpression&&) = default;
            
            CPNMultiSet eval(const Binding &binding) const;
            const std::set<Variable_t>& getVariables() const;
        private:
            ParameterizedColorSequenceMultiSet _parameterizedColorSequenceMultiSet;
            std::set<Variable_t> _variables;
        };

    }
}

#endif