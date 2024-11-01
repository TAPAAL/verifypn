#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H

#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/ExplicitColored/Algorithms/ColoredModelChecker.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"

namespace ColoredLTL{
        enum class ConditionalBool {
            FALSE,
            TRUE,
            UNKNOWN
        };

        enum class Quantifier {
            EF,
            AG
        };

        class NaiveWorklist {
        public:
            NaiveWorklist(
                const PetriEngine::ExplicitColored::ColoredPetriNet& net,
                const PetriEngine::PQL::Condition_ptr &query,
                std::unordered_map<std::string, uint32_t> placeNameIndices
            );

            bool check();

            ConditionalBool check(const PetriEngine::ExplicitColored::ColoredPetriNetMarking& state, ConditionalBool deadlockValue);

        private:
            PetriEngine::PQL::Condition_ptr _gammaQuery;
            Quantifier _quantifier;
            const std::unordered_map<std::string, uint32_t> _placeNameIndices;
            const PetriEngine::ExplicitColored::ColoredPetriNet& _net;

            bool dfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, const PetriEngine::ExplicitColored::ColoredPetriNetMarking& state);
        };
}
#endif //NAIVEWORKLIST_H