#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"

namespace PetriEngine::ExplicitColored {
    void ColoredResultPrinter::printResults(
        const SearchStatistics& searchStatistics,
        const Reachability::AbstractHandler::Result result
    ) const {
        if (result == Reachability::AbstractHandler::Unknown) {
            return;
        }
        _stream << "FORMULA " << _queryNames[_queryOffset]  << " ";
        if (result == Reachability::AbstractHandler::Satisfied) {
            _stream << "TRUE ";
        } else if (result == Reachability::AbstractHandler::NotSatisfied) {
            _stream << "FALSE ";
        }
        _stream << "TECHNIQUES ";
        for (const auto& techniqueFlag : _techniqueFlags) {
            _stream << techniqueFlag << " ";
        }
        _stream << std::endl;
        if (result == Reachability::AbstractHandler::Satisfied || result == Reachability::AbstractHandler::NotSatisfied) {
            _stream << "Query index " << _queryOffset << " was solved" << std::endl;
        }
        _stream << std::endl;

        _stream << "Query is ";
        if (result == Reachability::AbstractHandler::NotSatisfied) {
            _stream << "NOT ";
        }

        _stream << "satisfied" << std::endl;

        _stream << "STATES:" << std::endl
            << "passed states: " << searchStatistics.passedCount << std::endl
            << "explored states: " << searchStatistics.exploredStates << std::endl
            << "checked states: " << searchStatistics.checkedStates << std::endl
            << "peak waiting states: " << searchStatistics.peakWaitingStates << std::endl
            << "end waiting states: " << searchStatistics.endWaitingStates << std::endl;
    }
}

