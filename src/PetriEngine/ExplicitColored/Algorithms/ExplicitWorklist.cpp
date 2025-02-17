#ifndef EXPLICITWORKLIST_CPP
#define EXPLICITWORKLIST_CPP
#include "PetriEngine/ExplicitColored/Algorithms/ExplicitWorklist.h"

#include <PetriEngine/options.h>
#include "PetriEngine/ExplicitColored/Visitors/GammaQueryVisitor.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredMarkingSet.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/ExplicitColored/Algorithms/ColoredSearchTypes.h"

namespace PetriEngine::ExplicitColored {
    ExplicitWorklist::ExplicitWorklist(
        const ColoredPetriNet& net,
        const PQL::Condition_ptr &query,
        const std::unordered_map<std::string, uint32_t>& placeNameIndices,
        const std::unordered_map<std::string, Transition_t>& transitionNameIndices,
        const IColoredResultPrinter& coloredResultPrinter,
        const size_t seed
    ) : _net(std::move(net)),
        _seed(seed),
        _coloredResultPrinter(coloredResultPrinter)
    {
        const GammaQueryCompiler queryCompiler(placeNameIndices, transitionNameIndices, _net);
        if (const auto efGammaQuery = dynamic_cast<PQL::EFCondition*>(query.get())) {
            _quantifier = Quantifier::EF;
            _gammaQuery = queryCompiler.compile(efGammaQuery->getCond());
        } else if (const auto agGammaQuery = dynamic_cast<PQL::AGCondition*>(query.get())) {
            _quantifier = Quantifier::AG;
            _gammaQuery = queryCompiler.compile(agGammaQuery->getCond());
        } else {
            throw base_error("Unsupported query quantifier");
        }
    }

    bool ExplicitWorklist::check(const SearchStrategy searchStrategy, const ColoredSuccessorGeneratorOption colored_successor_generator_option) {
        if (colored_successor_generator_option == ColoredSuccessorGeneratorOption::FIXED) {
            return _search<ColoredPetriNetStateFixed>(searchStrategy);
        }
        if (colored_successor_generator_option == ColoredSuccessorGeneratorOption::EVEN) {
            return _search<ColoredPetriNetStateEven>(searchStrategy);
        }
        if (colored_successor_generator_option == ColoredSuccessorGeneratorOption::CONSTRAINED) {
            return _search<ColoredPetriNetStateConstrained>(searchStrategy);
        }
        throw base_error("Unsupported successor generator");
    }

    const SearchStatistics & ExplicitWorklist::GetSearchStatistics() const {
        return _searchStatistics;
    }

    bool ExplicitWorklist::_check(const ColoredPetriNetMarking& state) const {
        return _gammaQuery->eval(_net, state);
    }

    template <template <typename> typename WaitingList, typename T>
    bool ExplicitWorklist::_genericSearch(WaitingList<T> waiting) {
        ColoredSuccessorGenerator successorGenerator(_net);
        ptrie::set<uint8_t> passed;
        std::vector<uint8_t> scratchpad;
        const auto& initialState = _net.initial();
        const auto earlyTerminationCondition = _quantifier == Quantifier::EF;
        size_t size = initialState.compressedEncode(scratchpad);

        if constexpr (std::is_same_v<T, ColoredPetriNetStateEven>) {
            auto initial = ColoredPetriNetStateEven{initialState, _net.getTransitionCount()};
            waiting.add(std::move(initial));
        } else if constexpr (std::is_same_v<T, ColoredPetriNetStateConstrained>) {
            auto initial = ColoredPetriNetStateConstrained {initialState};
            waiting.add(std::move(initial));
        }else {
            auto initial = ColoredPetriNetStateFixed{initialState};
            waiting.add(std::move(initial));
        }
        passed.insert(scratchpad.data(), size);
        _searchStatistics.passedCount = 1;
        _searchStatistics.checkedStates = 1;

        if (_check(initialState) == earlyTerminationCondition) {
            return _getResult(true);
        }

        while (!waiting.empty()){
            auto& next = waiting.next();
            auto successor = successorGenerator.next(next);
            if (next.done()) {
                waiting.remove();
                continue;
            }

            if constexpr (std::is_same_v<T, ColoredPetriNetStateEven>) {
                if (next.shuffle){
                    next.shuffle = false;
                    waiting.shuffle();
                    continue;
                }
            }

            auto& marking = successor.marking;
            size = marking.compressedEncode(scratchpad);
            _searchStatistics.exploredStates++;
            if (!passed.exists(scratchpad.data(), size).first) {
                _searchStatistics.checkedStates += 1;

                if (_check(marking) == earlyTerminationCondition) {
                    _searchStatistics.endWaitingStates = waiting.size();
                    return _getResult(true);
                }
                successor.shrink();
                waiting.add(std::move(successor));
                passed.insert(scratchpad.data(), size);
                _searchStatistics.passedCount += 1;
                _searchStatistics.peakWaitingStates = std::max(waiting.size(), _searchStatistics.peakWaitingStates);
            }
        }
        _searchStatistics.endWaitingStates = waiting.size();
        return _getResult(false);
    }

    template<typename SuccessorGeneratorState>
    bool ExplicitWorklist::_search(const SearchStrategy searchStrategy) {
        switch (searchStrategy) {
            case SearchStrategy::DFS:
                return _dfs<SuccessorGeneratorState>();
            case SearchStrategy::BFS:
                return _bfs<SuccessorGeneratorState>();
            case SearchStrategy::RDFS:
                return _rdfs<SuccessorGeneratorState>();
            case SearchStrategy::HEUR:
                return _bestfs<SuccessorGeneratorState>();
            default:
                throw base_error("Unsupported exploration type");
        }
    }

    template <typename T>
    bool ExplicitWorklist::_dfs() {
        return _genericSearch<DFSStructure>(DFSStructure<T> {});
    }

    template <typename T>
    bool ExplicitWorklist::_bfs() {
        return _genericSearch<BFSStructure>(BFSStructure<T> {});
    }

    template <typename T>
    bool ExplicitWorklist::_rdfs() {
        return _genericSearch<RDFSStructure>(RDFSStructure<T>(_seed));
    }

    template <typename T>
    bool ExplicitWorklist::_bestfs() {
        return _genericSearch<BestFSStructure>(
            BestFSStructure<T>(
                _seed,
                _gammaQuery,
                _quantifier == Quantifier::AG
                )
            );
    }

    bool ExplicitWorklist::_getResult(const bool found) const {
        const auto res = (
            (!found && _quantifier == Quantifier::AG) ||
            (found && _quantifier == Quantifier::EF))
                ? Reachability::ResultPrinter::Result::Satisfied
                : Reachability::ResultPrinter::Result::NotSatisfied;
        _coloredResultPrinter.printResults(_searchStatistics, res);
        return res == Reachability::ResultPrinter::Result::Satisfied;
    }
}

#endif //NAIVEWORKLIST_CPP