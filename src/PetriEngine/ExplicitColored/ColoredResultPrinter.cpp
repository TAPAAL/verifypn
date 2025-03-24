#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"

namespace PetriEngine::ExplicitColored {
    void ColoredResultPrinter::printResult(
        const SearchStatistics& searchStatistics,
        const Reachability::AbstractHandler::Result result
    ) const {
        _printCommon(result, {});
        _stream << "STATES:" << std::endl
            << "passed states: " << searchStatistics.passedCount << std::endl
            << "explored states: " << searchStatistics.exploredStates << std::endl
            << "checked states: " << searchStatistics.checkedStates << std::endl
            << "peak waiting states: " << searchStatistics.peakWaitingStates << std::endl
            << "end waiting states: " << searchStatistics.endWaitingStates << std::endl;
    }

    void ColoredResultPrinter::printNonExplicitResult(std::vector<std::string> techniques,
        Reachability::AbstractHandler::Result result) const {
        _printCommon(result, techniques);
    }

    void ColoredResultPrinter::_printCommon(Reachability::AbstractHandler::Result result, const std::vector<std::string>& extraTechniques) const {
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
            _stream << "Query index " << _queryOffset << " was solved" << std::endl;
        }
        _stream << std::endl;

        _stream << "Query is ";
        if (result == Reachability::AbstractHandler::NotSatisfied) {
            _stream << "NOT ";
        }

        _stream << "satisfied" << std::endl;
    }
}

