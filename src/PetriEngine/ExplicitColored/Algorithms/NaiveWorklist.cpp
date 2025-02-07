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
#include "PetriEngine/ExplicitColored/FireabilityChecker.h"
#include <fstream>

namespace PetriEngine {
    namespace ExplicitColored {
        NaiveWorklist::NaiveWorklist(
            const ColoredPetriNet& net,
            const PQL::Condition_ptr &query,
            const std::unordered_map<std::string, uint32_t>& placeNameIndices,
            const std::unordered_map<std::string, Transition_t>& transitionNameIndices,
            const IColoredResultPrinter& coloredResultPrinter
        ) : _net(std::move(net)), _coloredResultPrinter(coloredResultPrinter), _placeNameIndices(placeNameIndices)
        {
            const GammaQueryCompiler queryCompiler(_placeNameIndices, transitionNameIndices, _net);
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

        bool NaiveWorklist::check(const SearchStrategy searchStrategy, const size_t seed) {
            switch (searchStrategy) {
                case SearchStrategy::DFS:
                    return _dfs<ColoredPetriNetStateV2>();
                case SearchStrategy::BFS:
                    return _bfs<ColoredPetriNetStateV2>();
                case SearchStrategy::RDFS:
                    return _rdfs<ColoredPetriNetStateV2>(seed);
                case SearchStrategy::HEUR:
                    return _bestfs<ColoredPetriNetStateV2>(seed);
                case SearchStrategy::EDFS:
                    return _dfs<ColoredPetriNetStateOneTrans>();
                case SearchStrategy::EBFS:
                    return _bfs<ColoredPetriNetStateOneTrans>();
                case SearchStrategy::ERDFS:
                    return _rdfs<ColoredPetriNetStateOneTrans>(seed);
                case SearchStrategy::EHEUR:
                    return _bestfs<ColoredPetriNetStateOneTrans>(seed);
                default:
                    throw base_error("Unsupported exploration type");
            }
        }

        const SearchStatistics & NaiveWorklist::GetSearchStatistics() const {
            return _searchStatistics;
        }

        bool NaiveWorklist::_check(const ColoredPetriNetMarking& state) const {
            return _gammaQuery->eval(_net, state);
        }

        template <template <typename> typename WaitingList, typename T>
        bool NaiveWorklist::_genericSearch(WaitingList<T> waiting) {
            ColoredSuccessorGenerator successorGenerator(_net);
            ptrie::set<uint8_t> passed;
            std::vector<uint8_t> scratchpad;
            const auto& initialState = _net.initial();
            const auto earlyTerminationCondition = _quantifier == Quantifier::EF;
            size_t size = initialState.compressedEncode(scratchpad);

            if constexpr (std::is_same_v<T, ColoredPetriNetStateOneTrans>) {
                std::cout << "EVEN" << std::endl;
                auto initial = ColoredPetriNetStateOneTrans{initialState, _net.getTransitionCount()};
                waiting.add(std::move(initial));
            }
            else
            {
                std::cout << "FIXED V2" << std::endl;
                auto initial = ColoredPetriNetStateV2{initialState};
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

                if constexpr (std::is_same_v<T, ColoredPetriNetStateOneTrans>) {
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