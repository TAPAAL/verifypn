#ifndef ARCCOMPILER_H
#define ARCCOMPILER_H

#include "AtomicTypes.h"
#include "../Colored/Expressions.h"
#include "CPNMultiSet.h"
#include "Binding.h"

namespace PetriEngine::ExplicitColored {
    union ColorOrVariable {
        Color_t color;
        Variable_t variable;
    };

    struct ParameterizedColor {
        ColorOffset_t offset;
        bool isVariable;
        //perhaps include "all" value so we dont need to materialize all colors
        ColorOrVariable value;
        bool operator==(const ParameterizedColor &other) const {
            return isVariable == other.isVariable
                && (isVariable
                    ? (value.variable == other.value.variable)
                    : (value.color == other.value.color)
                    ) && offset == other.offset;
        }

        bool operator!=(const ParameterizedColor &other) const {
            return !(*this == other);
        }

        bool isAll() const {
            return !isVariable && value.color == ALL_COLOR;
        }

        static ParameterizedColor fromColor(const Color_t color) {
            ParameterizedColor rv{};
            rv.isVariable = false;
            rv.offset = 0;
            rv.value.color = color;
            return rv;
        }

        static ParameterizedColor fromVariable(const Variable_t variable) {
            ParameterizedColor rv{};
            rv.isVariable = true;
            rv.offset = 0;
            rv.value.variable = variable;
            return rv;
        }

        static ParameterizedColor fromAll() {
            ParameterizedColor rv{};
            rv.isVariable = false;
            rv.offset = 0;
            rv.value.color = ALL_COLOR;
            return rv;
        }
    };

    class CompiledArcExpression {
    public:
        [[nodiscard]] virtual const CPNMultiSet& eval(const Binding& binding) const = 0;
        virtual void produce(CPNMultiSet& out, const Binding& binding) const = 0;
        virtual void consume(CPNMultiSet& out, const Binding& binding) const = 0;
        [[nodiscard]] virtual bool isSubSet(const CPNMultiSet& superSet, const Binding& binding) const {
            const auto& result = eval(binding);
            return result <= superSet;
        }
        [[nodiscard]] virtual MarkingCount_t getMinimalMarkingCount() const = 0;
        [[nodiscard]] virtual const std::set<Variable_t>& getVariables() const = 0;
        virtual ~CompiledArcExpression() = default;
    };

    class ArcCompiler {
    public:
        ArcCompiler(
            const std::unordered_map<std::string, Variable_t>& variableMap,
            const Colored::ColorTypeMap& colorTypeMap
        ) : _variableMap(std::move(variableMap)), _colorTypeMap(std::move(colorTypeMap)) {}

        [[nodiscard]] std::unique_ptr<CompiledArcExpression> compile(
            const Colored::ArcExpression_ptr& arcExpression
        ) const;

        static std::unique_ptr<CompiledArcExpression> testCompile();
    private:

        static void replaceConstants(std::unique_ptr<CompiledArcExpression>& top);
        static void preCalculate(std::unique_ptr<CompiledArcExpression>& top);

        const std::unordered_map<std::string, Variable_t>& _variableMap;
        const Colored::ColorTypeMap& _colorTypeMap;
    };
}



#endif //ARCCOMPILER_H
