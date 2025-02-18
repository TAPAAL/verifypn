#ifndef NAIVEWORKLIST_H
#define NAIVEWORKLIST_H


#include <PetriEngine/ExplicitColored/GammaQueryCompiler.h>
#include <PetriEngine/options.h>
#include "PetriEngine/ExplicitColored/ColoredPetriNet.h"
#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include "PetriEngine/ExplicitColored/SearchStatistics.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"


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

    class ExplicitWorklist {
    public:
        ExplicitWorklist(
            const ColoredPetriNet& net,
            const PQL::Condition_ptr &query,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const std::unordered_map<std::string, Transition_t>& transitionNameIndices,
            const IColoredResultPrinter& coloredResultPrinter,
            size_t seed
        );

        bool check(SearchStrategy searchStrategy, ColoredSuccessorGeneratorOption colored_successor_generator_option);
        [[nodiscard]] const SearchStatistics& GetSearchStatistics() const;
    private:
        std::shared_ptr<CompiledGammaQueryExpression> _gammaQuery;
        Quantifier _quantifier;
        const ColoredPetriNet& _net;
        const ColoredSuccessorGenerator _successorGenerator;
        const size_t _seed;
        bool _fullStatespace = true;
        SearchStatistics _searchStatistics;
        const IColoredResultPrinter& _coloredResultPrinter;
        template<typename SuccessorGeneratorState>
        [[nodiscard]] bool _search(SearchStrategy searchStrategy);
        [[nodiscard]] bool _check(const ColoredPetriNetMarking& state) const;

        template <typename T>
        [[nodiscard]] bool _dfs();
        template <typename T>
        [[nodiscard]] bool _bfs();
        template <typename T>
        [[nodiscard]] bool _rdfs();
        template <typename T>
        [[nodiscard]] bool _bestfs();

        template <template <typename> typename WaitingList, typename T>
        [[nodiscard]] bool _genericSearch(WaitingList<T> waiting);
        [[nodiscard]] bool _getResult(bool found) const;


    };
}

#endif //NAIVEWORKLIST_H