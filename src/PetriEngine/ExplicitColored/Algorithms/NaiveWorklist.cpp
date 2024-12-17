#ifndef NAIVEWORKLIST_CPP
#define NAIVEWORKLIST_CPP

#include <limits>
#include <PetriEngine/options.h>
#include "PetriEngine/ExplicitColored/Visitors/GammaQueryVisitor.h"
#include "PetriEngine/ExplicitColored/ColoredSuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ExplicitColored/ColoredPetriNetMarking.h"
#include "PetriEngine/ExplicitColored/ColoredMarkingSet.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/ExplicitColored/Algorithms/ColoredSearchTypes.h"
#include <fstream>

namespace PetriEngine {
    namespace ExplicitColored {
        NaiveWorklist::NaiveWorklist(
            const ColoredPetriNet& net,
            const PQL::Condition_ptr &query,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const IColoredResultPrinter& coloredResultPrinter
        ) : _net(std::move(net)),
            _placeNameIndices(placeNameIndices),
            _coloredResultPrinter(coloredResultPrinter)
        {
            if (const auto efGammaQuery = dynamic_cast<PQL::EFCondition*>(query.get())) {
                _quantifier = Quantifier::EF;
                _gammaQuery = efGammaQuery->getCond();
            } else if (const auto agGammaQuery = dynamic_cast<PQL::AGCondition*>(query.get())) {
                _quantifier = Quantifier::AG;
                _gammaQuery = agGammaQuery->getCond();
            } else {
                throw base_error("Unsupported query quantifier");
            }
        }

        bool NaiveWorklist::check(const SearchStrategy searchStrategy, const size_t seed) {
            switch (searchStrategy) {
                case SearchStrategy::DFS:
                    return _dfs<ColoredPetriNetState>();
                case SearchStrategy::BFS:
                    return _bfs<ColoredPetriNetState>();
                case SearchStrategy::RDFS:
                    return _rdfs<ColoredPetriNetStateOneTrans>(seed);
                case SearchStrategy::BESTFS:
                    return _bestfs<ColoredPetriNetState>(seed);
                default:
                    throw base_error("Unsupported exploration type");
            }
        }

        const SearchStatistics & NaiveWorklist::GetSearchStatistics() const {
            return _searchStatistics;
        }

        ConditionalBool NaiveWorklist::_check(const ColoredPetriNetMarking& state, const ConditionalBool deadlockValue) const {
            return GammaQueryVisitor::eval(_gammaQuery, state, _placeNameIndices, deadlockValue);
        }

        template <template <typename> typename WaitingList, typename T>
        bool NaiveWorklist::_genericSearch(WaitingList<T> waiting) {
            auto passed = ColoredMarkingSet {};
            ColoredSuccessorGenerator successorGenerator(_net);
            const auto& initialState = _net.initial();

            if constexpr (std::is_same_v<WaitingList<T>, RDFSStructure<T>>) {
                auto initial = ColoredPetriNetStateOneTrans{initialState, _net.nTransitions()};
                waiting.add(std::move(initial));
            }else {
                auto initial = ColoredPetriNetState{initialState};
                waiting.add(std::move(initial));
            }
            passed.add(initialState);
            _searchStatistics.passedCount = 1;
            _searchStatistics.checkedStates = 1;
            const auto earlyTerminationCondition = (_quantifier == Quantifier::EF)
                ? ConditionalBool::TRUE
                : ConditionalBool::FALSE;

            if (_check(initialState, ConditionalBool::UNKNOWN) == earlyTerminationCondition) {
                return _getResult(true);
            }
            while (!waiting.empty()) {
                auto& next = waiting.next();
                auto successor = successorGenerator.next(next);

                if (next.done){
                    waiting.remove();
                    if constexpr (std::is_same_v<WaitingList<T>, RDFSStructure<T>>) {
                        waiting.shuffle();
                    }
                    continue;
                }

                if constexpr (std::is_same_v<WaitingList<T>, RDFSStructure<T>>) {
                    if (next.shuffle){
                        next.shuffle = false;
                        next.skip = false;
                        waiting.shuffle();
                        continue;
                    }
                }

                if constexpr (std::is_same_v<T, ColoredPetriNetStateOneTrans>) {
                    if (next.skip) {
                        next.skip = false;
                        continue;
                    }
                }
                auto marking = successor.marking;
                _searchStatistics.exploredStates++;
                if (!passed.contains(marking)) {
                    _searchStatistics.checkedStates += 1;
                    if (_check(marking, ConditionalBool::UNKNOWN) == earlyTerminationCondition) {
                        _searchStatistics.passedCount = passed.size();
                        _searchStatistics.endWaitingStates = waiting.size();
                        return _getResult(true);
                    }
                    successor.shrink();
                    passed.add(std::move(marking));
                    waiting.add(std::move(successor));
                    _searchStatistics.passedCount = passed.size();
                    _searchStatistics.endWaitingStates = waiting.size();
                    _searchStatistics.peakWaitingStates = std::max(waiting.size(), _searchStatistics.peakWaitingStates);
                }
            }
            _searchStatistics.passedCount = passed.size();
            _searchStatistics.endWaitingStates = waiting.size(); //0
            return _getResult(false);
        }

        template <typename T>
        bool NaiveWorklist::_dfs() {
            return _genericSearch<DFSStructure>(DFSStructure<T> {});
        }

        template <typename T>
        bool NaiveWorklist::_bfs() {
            return _genericSearch<BFSStructure>(BFSStructure<T> {});
        }

        template <typename T>
        bool NaiveWorklist::_rdfs(const size_t seed) {
            return _genericSearch<RDFSStructure>(RDFSStructure<T>(seed));
        }

        template <typename T>
        bool NaiveWorklist::_bestfs(const size_t seed) {
            return _genericSearch<BestFSStructure>(
                BestFSStructure<T>(
                    seed,
                    _gammaQuery,
                    _placeNameIndices,
                    _quantifier == Quantifier::AG
                    )
                );
        }

        bool NaiveWorklist::_getResult(const bool found) const {
            const auto res = (
                (!found && _quantifier == Quantifier::AG) ||
                (found && _quantifier == Quantifier::EF))
                    ? Reachability::ResultPrinter::Result::Satisfied
                    : Reachability::ResultPrinter::Result::NotSatisfied;
            _coloredResultPrinter.printResults(_searchStatistics, res);
            return res == Reachability::ResultPrinter::Result::Satisfied;
        }
    }
}
#endif //NAIVEWORKLIST_CPP