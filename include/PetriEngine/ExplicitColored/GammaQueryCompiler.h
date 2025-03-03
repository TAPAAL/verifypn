//
// Created by joms on 2/3/25.
//

#ifndef GAMMAQUERYCOMPILER_H
#define GAMMAQUERYCOMPILER_H
#include <PetriEngine/PQL/PQL.h>

#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"

namespace PetriEngine::ExplicitColored {
    class CompiledGammaQueryExpression {
    public:
        virtual ~CompiledGammaQueryExpression() = default;
        [[nodiscard]] virtual bool eval(const ColoredSuccessorGenerator& successorGenerator, const ColoredPetriNetMarking& marking, size_t id) const = 0;
        [[nodiscard]] virtual MarkingCount_t distance(const ColoredPetriNetMarking& marking, bool neg) const = 0;
    };

    class GammaQueryCompiler {
    public:
        GammaQueryCompiler(
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
            const ColoredSuccessorGenerator& successorGenerator
        );

        [[nodiscard]] std::unique_ptr<CompiledGammaQueryExpression> compile(const PQL::Condition_ptr &expression) const;
    private:
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
        const std::unordered_map<std::string, uint32_t>& _transitionNameIndices;
        ColoredSuccessorGenerator _successorGenerator;
    };
}

#endif //GAMMAQUERYCOMPILER_H

