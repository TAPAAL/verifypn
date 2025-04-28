#ifndef EXPLICITQUERYPROPOSITIONCOMPILER_H
#define EXPLICITQUERYPROPOSITIONCOMPILER_H

#include <PetriEngine/PQL/PQL.h>
#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/ExplicitColored/SuccessorGenerator/ColoredSuccessorGenerator.h"

namespace PetriEngine::ExplicitColored {
    class ExplicitQueryProposition {
    public:
        virtual ~ExplicitQueryProposition() = default;
        [[nodiscard]] virtual bool eval(const ColoredSuccessorGenerator& successorGenerator,
                                        const ColoredPetriNetMarking& marking, size_t id) const = 0;
        [[nodiscard]] virtual MarkingCount_t distance(const ColoredPetriNetMarking& marking, bool neg) const = 0;
    };

    class ExplicitQueryPropositionCompiler {
    public:
        ExplicitQueryPropositionCompiler(
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const std::unordered_map<std::string, uint32_t>& transitionNameIndices,
            const ColoredSuccessorGenerator& successorGenerator
        );

        [[nodiscard]] std::unique_ptr<ExplicitQueryProposition> compile(const PQL::Condition_ptr& expression) const;

    private:
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
        const std::unordered_map<std::string, uint32_t>& _transitionNameIndices;
        ColoredSuccessorGenerator _successorGenerator;
    };
}

#endif //EXPLICITQUERYPROPOSITIONCOMPILER_H

