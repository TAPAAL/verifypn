#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H


#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include "PetriEngine/ExplicitColored/SearchStatistics.h"


namespace PetriEngine::ExplicitColored {
    template <typename T>
    class RDFSStructure;
    class ColoredResultPrinter;

    enum class Quantifier {
        EF,
        AG
    };

    enum class SearchStrategy {
        DFS,
        BFS,
        RDFS,
        HEUR
    };

    class NaiveWorklist {
    public:
        NaiveWorklist(
            const ColoredPetriNet& net,
            const PQL::Condition_ptr &query,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const std::unordered_map<std::string, Transition_t>& transitionNameIndices,
            const IColoredResultPrinter& coloredResultPrinter,
            size_t seed
        );

        bool check(SearchStrategy searchStrategy, ColoredSuccessorGeneratorOption colored_successor_generator_option);
        const SearchStatistics& GetSearchStatistics() const;
    private:
        PQL::Condition_ptr _gammaQuery;
        Quantifier _quantifier;
        const ColoredPetriNet& _net;
        const std::unordered_map<std::string, uint32_t>& _placeNameIndices;
        const std::unordered_map<std::string, Transition_t> _transitionNameIndices;
        const size_t _seed;

        template<typename SuccessorGeneratorState>
        bool _search(SearchStrategy searchStrategy);
        bool _check(const ColoredPetriNetMarking& state) const;

        template <typename T>
        bool _dfs();
        template <typename T>
        bool _bfs();
        template <typename T>
        bool _rdfs();
        template <typename T>
        bool _bestfs();

        template <template <typename> typename WaitingList, typename T>
        bool _genericSearch(WaitingList<T> waiting);
        bool _getResult(bool found) const;

        SearchStatistics _searchStatistics;
        const IColoredResultPrinter& _coloredResultPrinter;
    };
}

#endif //NAIVEWORKLIST_H