#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"

#include <iomanip>

namespace PetriEngine::ExplicitColored {
    void ColoredResultPrinter::printResult(
        const SearchStatistics& searchStatistics,
        const Reachability::AbstractHandler::Result result,
        const std::vector<TraceStep>* trace
    ) const {
        _printCommon(result, {});
        _stream << "STATS:" << std::endl
                << "	discovered states:     " << searchStatistics.discoveredStates << std::endl
                << "	explored states:       " << searchStatistics.exploredStates << std::endl
                << "	peak waiting states:   " << searchStatistics.peakWaitingStates << std::endl
                << "	end waiting states:    " << searchStatistics.endWaitingStates << std::endl
                << "	biggest encoded state: " << searchStatistics.biggestEncoding << " bytes" << std::endl;
        if (trace != nullptr) {
            _printTrace(*trace);
        }
    }

    void ColoredResultPrinter::printNonExplicitResult(const std::vector<std::string> techniques,
        const Reachability::AbstractHandler::Result result) const {
        _printCommon(result, techniques);
    }

    void ColoredResultPrinter::_printCommon(const Reachability::AbstractHandler::Result result, const std::vector<std::string>& extraTechniques) const {
        if (result == Reachability::AbstractHandler::Unknown) {
            return;
        }
        std::cout << "FORMULA " << _queryName  << " ";
        if (result == Reachability::AbstractHandler::Satisfied) {
            std::cout << "TRUE ";
        } else if (result == Reachability::AbstractHandler::NotSatisfied) {
            std::cout << "FALSE ";
        }

        std::cout << "TECHNIQUES ";
        for (const auto& techniqueFlag : _techniqueFlags) {
            std::cout << techniqueFlag << " ";
        }

        for (const auto& techniqueFlag : extraTechniques) {
            std::cout << techniqueFlag << " ";
        }

        std::cout << std::endl;
        if (result == Reachability::AbstractHandler::Satisfied || result == Reachability::AbstractHandler::NotSatisfied) {
            std::cout << "Query index " << _queryOffset << " was solved" << std::endl;
        }
        std::cout << std::endl;

        std::cout << "Query is ";
        if (result == Reachability::AbstractHandler::NotSatisfied) {
            std::cout << "NOT ";
        }

        std::cout << "satisfied" << std::endl;
    }

    void ColoredResultPrinter::_printTrace(const std::vector<TraceStep>& trace) const {
        _traceStream << "Trace: " << std::endl;
        _traceStream << "<trace>" << std::endl;
        for (const auto& step : trace) {
            if (!step.isInitial) {
                _traceStream << "\t<transition id=" << std::quoted(step.transitionId) << ">" << std::endl;
                _traceStream << "\t\t<bindings>" << std::endl;
                for (const auto& [variableId, value] : step.binding) {
                    _traceStream << "\t\t\t<variable id=" << std::quoted(variableId) << ">" << std::endl;
                    _traceStream << "\t\t\t\t<color>" << value << "</color>" << std::endl;
                    _traceStream << "\t\t\t</variable>" << std::endl;
                }
                _traceStream << "\t\t</bindings>" << std::endl;
                _traceStream << "\t</transition>" << std::endl;
            }
            _traceStream << "\t<marking>" << std::endl;
            for (const auto& [placeId, marking] : step.marking) {
                if (marking.size() > 0) {
                    _traceStream << "\t\t<place id=" << std::quoted(placeId) << ">" << std::endl;
                    for (const auto& [productColor, count] : marking) {
                        if (count > 0) {
                            _traceStream << "\t\t\t<token count=" << std::quoted(std::to_string(count)) << ">" << std::endl;
                            for (const auto& color : productColor) {
                                _traceStream << "\t\t\t\t<color>" << color << "</color>" << std::endl;
                            }
                            _traceStream << "\t\t\t</token>" << std::endl;
                        }
                    }
                    _traceStream << "\t\t</place>" << std::endl;
                }
            }
            _traceStream << "\t</marking>" << std::endl;
        }
        _traceStream << "</trace>" << std::endl;
    }
}

