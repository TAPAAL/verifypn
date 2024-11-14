#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H

#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"

namespace PetriEngine {
    namespace ExplicitColored {
        enum class ConditionalBool {
            FALSE,
            TRUE,
            UNKNOWN
        };

        enum class Quantifier {
            EF,
            AG
        };

        enum class SearchStrategy {
            DFS,
            BFS
        };

        class NaiveWorklist {
        public:
            NaiveWorklist(
                const ColoredPetriNet& net,
                const PQL::Condition_ptr &query,
                std::unordered_map<std::string, uint32_t> placeNameIndices
            );

            bool check(SearchStrategy searchStrategy);
        private:
            PQL::Condition_ptr _gammaQuery;
            Quantifier _quantifier;
            const ColoredPetriNet& _net;
            const std::unordered_map<std::string, uint32_t> _placeNameIndices;

            ConditionalBool _check(const PetriEngine::ExplicitColored::ColoredPetriNetMarking& state, ConditionalBool deadlockValue);

            bool _dfs();
            bool _bfs();

            template<typename WaitingList>
            bool _genericSearch();
        };
    }
}
#endif //NAIVEWORKLIST_H