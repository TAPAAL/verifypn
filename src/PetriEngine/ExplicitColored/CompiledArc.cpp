//
// Created by joms on 12/3/24.
//

#include "PetriEngine/ExplicitColored/CompiledArc.h"

namespace PetriEngine {
    namespace ExplicitColored {
        CPNMultiSet CompiledArc::eval(const Binding& binding) const {
            CPNMultiSet result(_constantValue);
            for (auto& [sequence, count] : _variableSequences) {
                auto colorSequence = getColorSequence(sequence, binding);
                result.addCount(ColorSequence(std::move(colorSequence)), count);
            }
            if (_variableSequences.size() > 0) {
                result.fixNegative();
            }
            return result;
        }

        void CompiledArc::produce(CPNMultiSet &existing, const Binding &binding) const {
            if (_variableSequences.empty()) {
                existing += _constantValue;
                return;
            }
            CPNMultiSet newValue = _constantValue;
            for (auto& [sequence, count] : _variableSequences) {
                auto colorSequence = getColorSequence(sequence, binding);
                newValue.addCount(ColorSequence(std::move(colorSequence)), count);
            }
            newValue.fixNegative();
            existing += newValue;
        }

        void CompiledArc::consume(CPNMultiSet &existing, const Binding &binding) const {
            if (_variableSequences.empty()) {
                existing -= _constantValue;
                return;
            }
            CPNMultiSet newValue = _constantValue;
            for (auto& [sequence, count] : _variableSequences) {
                auto colorSequence = getColorSequence(sequence, binding);
                newValue.addCount(ColorSequence(std::move(colorSequence)), count);
            }
            newValue.fixNegative();
            existing -= newValue;
        }

        bool CompiledArc::isSubSet(const CPNMultiSet &superSet, const Binding &binding) const {
            if (_variableSequences.empty()) {
                return superSet >= _constantValue;
            }
            CPNMultiSet newValue = _constantValue;
            for (auto& [sequence, count] : _variableSequences) {
                auto colorSequence = getColorSequence(sequence, binding);
                newValue.addCount(ColorSequence(std::move(colorSequence)), count);
            }
            return superSet >= newValue;
        }

        std::vector<Color_t> CompiledArc::getColorSequence(
            const std::vector<ParameterizedColor> &parameterizedColorSequence, const Binding &binding) const {
            std::vector<Color_t> colorSequence;
            colorSequence.reserve(_sequenceColorSizes.size());
            for (size_t i = 0; i < parameterizedColorSequence.size(); i++) {
                const ParameterizedColor& color = parameterizedColorSequence[i];
                int64_t colorValue;
                if (color.isVariable) {
                    colorValue = binding.getValue(color.value.variable) + color.offset;
                } else {
                    colorValue = (color.value.color) + color.offset;
                }
                //Keeps the value between 0 - max with correct wrap around handling
                colorSequence.push_back(signed_wrap(colorValue, _sequenceColorSizes[i]));
            }
            return colorSequence;
        }
    }
}