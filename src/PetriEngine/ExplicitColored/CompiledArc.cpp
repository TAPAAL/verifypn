#include "PetriEngine/ExplicitColored/CompiledArc.h"

#include <numeric>

namespace PetriEngine {
    namespace ExplicitColored {
        CompiledArc::CompiledArc(
            std::vector<std::pair<std::vector<ParameterizedColor>, sMarkingCount_t>> variableSequences,
            CPNMultiSet constantValue,
            std::vector<Color_t> sequenceColorSizes,
            std::set<Variable_t> variables
        ) : _variableSequences(std::move(variableSequences)),
            _constantValue(std::move(constantValue)),
            _sequenceColorSizes(std::move(sequenceColorSizes)),
            _variables(std::move(variables))
        {
            hasNegative =
                std::any_of(_constantValue.counts().begin(), _constantValue.counts().end(),
                    [](const auto& p) {return p.second < 0;})
                || std::any_of(_variableSequences.begin(), _variableSequences.end(),
                    [](const auto& p) { return p.second < 0;});

            _minimalMarkingCount = std::max(std::accumulate(_constantValue.counts().begin(), _constantValue.counts().end(), 0,
                    [](const auto& v, const auto& p) {return v + p.second;}) +
                std::accumulate(_variableSequences.begin(), _variableSequences.end(), 0,
                    [](const auto& v, const auto& p) {return v + p.second;}), 0);

            _burner.getSequence().resize(_sequenceColorSizes.size());
        }

        CPNMultiSet CompiledArc::eval(const Binding& binding) const {
            CPNMultiSet result(_constantValue);
            addVariables(result, binding);
            if (hasNegative) {
                result.fixNegative();
            }
            return result;
        }

        void CompiledArc::produce(CPNMultiSet &existing, const Binding &binding) const {
            if (_variableSequences.empty()) {
                existing += _constantValue;
                return;
            }
            if (!hasNegative) {
                existing += _constantValue;
                addVariables(existing, binding);
            } else {
                CPNMultiSet newValue = _constantValue;
                addVariables(newValue, binding);
                newValue.fixNegative();
                existing += newValue;
            }
        }

        void CompiledArc::consume(CPNMultiSet &existing, const Binding &binding) const {
            if (_variableSequences.empty()) {
                existing -= _constantValue;
                return;
            }
            if (!hasNegative) {
                existing -= _constantValue;
                for (auto& [sequence, count] : _variableSequences) {
                    setBurnerSequence(sequence, binding);
                    existing.addCount(_burner, -count);
                }
                existing.fixNegative();
            } else {
                CPNMultiSet newValue = _constantValue;
                addVariables(newValue, binding);
                newValue.fixNegative();
                existing -= newValue;
            }
        }

        bool CompiledArc::isSubSet(const CPNMultiSet &superSet, const Binding &binding) const {
            if (_variableSequences.empty()) {
                return superSet >= _constantValue;
            }
            if (!hasNegative) {
                CPNMultiSet newValue;
                addVariables(newValue, binding);
                return superSet >= _constantValue && superSet >= newValue;
            }
            CPNMultiSet newValue = _constantValue;
            addVariables(newValue, binding);
            return superSet >= newValue;
        }

        MarkingCount_t CompiledArc::getMinimalMarkingCount() const {
            return _minimalMarkingCount;
        }

        void CompiledArc::setBurnerSequence(
            const std::vector<ParameterizedColor> &parameterizedColorSequence,
            const Binding &binding
        ) const {
            for (size_t i = 0; i < parameterizedColorSequence.size(); i++) {
                const ParameterizedColor& color = parameterizedColorSequence[i];
                int64_t colorValue;
                if (color.isVariable) {
                    colorValue = binding.getValue(color.value.variable) + color.offset;
                } else {
                    colorValue = (color.value.color) + color.offset;
                }
                //Keeps the value between 0 - max with correct wrap around handling
                _burner.getSequence()[i] = signed_wrap(colorValue, _sequenceColorSizes[i]);
            }
        }

        void CompiledArc::addVariables(CPNMultiSet &target, const Binding& binding) const {
            for (auto& [sequence, count] : _variableSequences) {
                setBurnerSequence(sequence, binding);
                target.addCount(_burner, count);
            }
        }
    }
}
