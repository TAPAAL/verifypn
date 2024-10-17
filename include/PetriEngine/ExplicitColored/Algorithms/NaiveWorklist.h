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
            const PetriEngine::PQL::Condition_ptr &query)
            : ColoredModelChecker(net, query){
            }

            virtual bool check();

            template<typename S>
            bool check(S state);

        private:
            using State = PetriEngine::ExplicitColored::ColoredPetriNetMarking;


            template<typename S>
            bool bfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, const S& state);

            template<typename S>
            bool dfs(PetriEngine::ExplicitColored::ColoredSuccessorGenerator& successor_generator, S& state);
        };
}
#endif //NAIVEWORKLIST_H