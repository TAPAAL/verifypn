#ifndef COMPILEDARC_H
#define COMPILEDARC_H
#include <vector>
#include "CPNMultiSet.h"
#include "Binding.h"
#include "utils/MathExt.h"

namespace PetriEngine {
    namespace ExplicitColored {
        union ColorOrVariable {
            Color_t color;
            Variable_t variable;
        };

        struct ParameterizedColor {
            int64_t offset;
            bool isVariable;
            //perhaps include "all" value so we dont need to materialize all colors
            ColorOrVariable value;
            bool operator==(const ParameterizedColor &other) const {
                return isVariable == other.isVariable
                    && (isVariable
                        ? (value.variable == other.value.variable)
                        : (value.color == other.value.color)
                        );
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

        class CompiledArc {
        public:
            explicit CompiledArc(
                std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>> _variableSequences,
                CPNMultiSet _constantValue,
                std::vector<Color_t> sequenceColorSizes,
                std::set<Variable_t> variables
            ) : _variableSequences(std::move(_variableSequences)),
                _constantValue(std::move(_constantValue)),
                _sequenceColorSizes(std::move(sequenceColorSizes)),
                _variables(std::move(variables))
             { }

            [[nodiscard]] CPNMultiSet eval(const Binding& binding) const;
            void produce(CPNMultiSet& existing, const Binding& binding) const;
            void consume(CPNMultiSet& existing, const Binding& binding) const;
            [[nodiscard]] bool isSubSet(const CPNMultiSet& superSet, const Binding& binding) const;
            [[nodiscard]] const std::set<Variable_t>& getVariables() const {
                return _variables;
            }
        private:
            [[nodiscard]] std::vector<Color_t> getColorSequence(const std::vector<ParameterizedColor>& parameterizedColorSequence, const Binding& binding) const;

            std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>> _variableSequences;
            CPNMultiSet _constantValue;
            std::vector<Color_t> _sequenceColorSizes;
            std::set<Variable_t> _variables;
        };
    }
}

#endif