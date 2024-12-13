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
                    return _dfs();
                case SearchStrategy::BFS:
                    return _bfs();
                case SearchStrategy::RDFS:
                    return _rdfs(seed);
                case SearchStrategy::BESTFS:
                    return _bestfs(seed);
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

        template<typename WaitingList>
        bool NaiveWorklist::_genericSearch(WaitingList waiting) {
            auto passed = ColoredMarkingSet {};
            const auto& initialState = _net.initial();
            auto initial = ColoredPetriNetState{initialState, std::is_same_v<WaitingList, RDFSStructure>};
            ColoredSuccessorGenerator successorGenerator(_net);
            waiting.add(std::move(initial));
            passed.add(initialState);
            _searchStatistics.passedCount = 1;
            _searchStatistics.checkedStates = 1;
            const auto earlyTerminationCondition = (_quantifier == Quantifier::EF)
                ? ConditionalBool::TRUE
                : ConditionalBool::FALSE;

            if (_check(initialState, ConditionalBool::UNKNOWN) == earlyTerminationCondition) {
                return _getResult(true);
            }
            while (!waiting.empty()){
                auto& next = waiting.next();
                auto successor = successorGenerator.next(next);
                if constexpr (std::is_same_v<WaitingList, RDFSStructure>) {
                    if (successor.lastBinding == std::numeric_limits<Binding_t>::max() && successor.lastTrans == std::numeric_limits<Transition_t>::max()){
                        waiting.remove();
                        waiting.shuffle();
                        continue;
                    }
                    if (successor.lastTrans == std::numeric_limits<Transition_t>::max()) {
                        if (!next.hasAdded) {
                            auto newState = ColoredPetriNetState(next);
                            ++newState.lastTrans;
                            newState.lastBinding = 0;
                            waiting.remove();
                            waiting.add(std::move(newState));
                        }else {
                            waiting.remove();
                        }
                        continue;
                    }
                }else {
                    if (successor.lastTrans ==  std::numeric_limits<Transition_t>::max()) {
                        waiting.remove();
                        continue;
                    }
                }
                auto& marking = successor.marking;
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
                    if constexpr (std::is_same_v<WaitingList, RDFSStructure>) {
                        successor.hasAdded = false;
                        waiting.add(std::move(successor));
                        if (!next.hasAdded) {
                            auto newState = ColoredPetriNetState(next);
                            ++newState.lastTrans;
                            newState.lastBinding = 0;
                            next.hasAdded = true;
                            waiting.add(std::move(newState));
                        }
                    }else {
                        waiting.add(std::move(successor));
                    }
                    passed.add(marking);
                    _searchStatistics.passedCount = passed.size();
                    _searchStatistics.endWaitingStates = waiting.size();
                    _searchStatistics.peakWaitingStates = std::max(waiting.size(), _searchStatistics.peakWaitingStates);
                }
            }
            _searchStatistics.passedCount = passed.size();
            _searchStatistics.endWaitingStates = waiting.size(); //0
            return _getResult(false);
        }

        bool NaiveWorklist::_dfs() {
            return _genericSearch<DFSStructure>(DFSStructure {});
        }

        bool NaiveWorklist::_bfs() {
            return _genericSearch<BFSStructure>(BFSStructure {});
        }

        bool NaiveWorklist::_rdfs(const size_t seed) {
            return _genericSearch<RDFSStructure>(RDFSStructure(seed));
        }

        bool NaiveWorklist::_bestfs(const size_t seed) {
            return _genericSearch<BestFSStructure>(
                BestFSStructure(
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