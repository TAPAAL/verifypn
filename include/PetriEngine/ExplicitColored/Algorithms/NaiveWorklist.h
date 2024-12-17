#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H


#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include "PetriEngine/ExplicitColored/SearchStatistics.h"

namespace PetriEngine {
    namespace ExplicitColored {
        template <typename T>
        class RDFSStructure;
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
            RDFS,
            BESTFS,
            RPFS,
        };

        class NaiveWorklist {
        public:
            NaiveWorklist(
                const ColoredPetriNet& net,
                const PQL::Condition_ptr &query,
                const std::unordered_map<std::string, uint32_t>& placeNameIndices,
                const IColoredResultPrinter& coloredResultPrinter
            );

            bool check(SearchStrategy searchStrategy, size_t seed);
            const SearchStatistics& GetSearchStatistics() const;
        private:
            PQL::Condition_ptr _gammaQuery;
            Quantifier _quantifier;
            const ColoredPetriNet& _net;
            const std::unordered_map<std::string, uint32_t>& _placeNameIndices;

            ConditionalBool _check(const ColoredPetriNetMarking& state, ConditionalBool deadlockValue) const;


            template <typename T>
            bool _dfs();
            template <typename T>
            bool _bfs();
            template <typename T>
            bool _rdfs(size_t seed);
            template <typename T>
            bool _bestfs(size_t seed);

            template <template <typename> typename WaitingList, typename T>
            bool _genericSearch(WaitingList<T> waiting);
            bool _getResult(bool found) const;

            SearchStatistics _searchStatistics;
            const IColoredResultPrinter& _coloredResultPrinter;


        };
    }
}
#endif //NAIVEWORKLIST_H