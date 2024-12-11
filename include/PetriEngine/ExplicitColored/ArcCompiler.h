#ifndef ARCCOMPILER_H
#define ARCCOMPILER_H

#include "CompiledArc.h"
#include "../Colored/Expressions.h"

namespace PetriEngine {
    namespace ExplicitColored {
        class ArcCompiler {
        public:
            static CompiledArc compileArc(
                const Colored::ArcExpression_ptr& arcExpression,
                const std::unordered_map<std::string, Variable_t>& variableMap,
                const Colored::ColorTypeMap& colorTypeMap
            );
        };
    }
}


#endif //ARCCOMPILER_H
