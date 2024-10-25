#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H

#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/ExplicitColored/Algorithms/ColoredModelChecker.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"

namespace ColoredLTL{
        class NaiveWorklist : public ColoredModelChecker {
        public:
            NaiveWorklist(const PetriEngine::ExplicitColored::ColoredPetriNet& net,
            const PetriEngine::PQL::Condition_ptr &query,
            std::unordered_map<std::string, uint32_t> placeNameIndices)
                : ColoredModelChecker(net, query), _placeNameIndices(std::move(placeNameIndices)) { }

            bool check() override;

            template<typename S>
            bool check(S state);

        private:
            using State = PetriEngine::ExplicitColored::ColoredPetriNetMarking;


            template<typename S>
            bool bfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, const S& state);

            template<typename S>
            bool dfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, S& state);

            const std::unordered_map<std::string, uint32_t> _placeNameIndices;
        };
}
#endif //NAIVEWORKLIST_H