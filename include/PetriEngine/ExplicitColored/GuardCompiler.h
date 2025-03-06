#ifndef GUARDCOMPILER_H
#define GUARDCOMPILER_H
#include <string>
#include <unordered_map>
#include "AtomicTypes.h"
#include "../Colored/Colors.h"
#include "../Colored/Expressions.h"
#include "Binding.h"
#include <memory>
#include "VariableExtractorVisitor.h"

namespace PetriEngine {
    namespace ExplicitColored {
        class CompiledGuardExpression {
        public:
            virtual bool eval(const Binding& binding) = 0;
            virtual void collectVariables(std::set<Variable_t>& out) const = 0;
            virtual ~CompiledGuardExpression() = default;
        };

        class GuardCompiler {
        public:
            GuardCompiler(const std::unordered_map<std::string, Variable_t>& variableMap, const Colored::ColorTypeMap& colorTypeMap);
            std::unique_ptr<CompiledGuardExpression> compile(const Colored::GuardExpression& colorExpression) const;
        private:
            const Colored::ColorTypeMap& _colorTypeMap;
            const std::unordered_map<std::string, Variable_t>& _variableMap;
        };
    }
}

#endif //GUARDCOMPILER_H
