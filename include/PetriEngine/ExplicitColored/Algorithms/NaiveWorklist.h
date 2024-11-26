#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H


#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include "PetriEngine/ExplicitColored/SearchStatistics.h"

namespace PetriEngine {
    namespace ExplicitColored {
        class ColoredResultPrinter;
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
            BFS,
            RDFS
        };

        class NaiveWorklist {
        public:
            NaiveWorklist(
                const ColoredPetriNet& net,
                const PQL::Condition_ptr &query,
                std::unordered_map<std::string, uint32_t> placeNameIndices,
                const IColoredResultPrinter& coloredResultPrinter
            );

            bool check(SearchStrategy searchStrategy, size_t randomSeed);
            const SearchStatistics& GetSearchStatistics() const;
        private:
            PQL::Condition_ptr _gammaQuery;
            Quantifier _quantifier;
            const ColoredPetriNet& _net;
            const std::unordered_map<std::string, uint32_t> _placeNameIndices;

            ConditionalBool _check(const PetriEngine::ExplicitColored::ColoredPetriNetMarking& state, ConditionalBool deadlockValue);

            bool _dfs();
            bool _bfs();
            bool _rdfs(size_t seed);

            template<typename WaitingList>
            bool _genericSearch(WaitingList waiting);

            bool getResult(bool found);

            SearchStatistics _searchStatistics;
            const IColoredResultPrinter& _coloredResultPrinter;
        };
    }
}
#endif //NAIVEWORKLIST_H