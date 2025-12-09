#include "PetriEngine/ExplicitColored/ColoredResultPrinter.h"
#include <iomanip>
#include "PetriEngine/Colored/PnmlWriter.h"


namespace PetriEngine::ExplicitColored {
    void ColoredResultPrinter::printResult(
        const SearchStatistics& searchStatistics,
        const Reachability::AbstractHandler::Result result,
        const ExplicitColoredTraceContext* trace
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

    void ColoredResultPrinter::_printTrace(const ExplicitColoredTraceContext& trace) const {
        _traceStream << "Trace: " << std::endl;
        _traceStream << "<trace>" << std::endl;
        for (const auto& step : trace.traceSteps) {
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
            _printMarkings(trace.cpnBuilder, step);
            _traceStream << "\t</marking>" << std::endl;
        }
        _traceStream << "</trace>" << std::endl;
    }

    void ColoredResultPrinter::_printMarkings(
        const ExplicitColoredPetriNetBuilder& explicitCpnBuilder, const TraceStep& traceStep) const
    {
        shared_string_set sharedStringSet {};
        ColoredPetriNetBuilder builder(sharedStringSet);
        for (const auto colorType : explicitCpnBuilder.getUnderlyingVariableColorTypes())
        {
            builder.addColorType(colorType->getName(), colorType);
        }

        for (const auto& [place_id, traceTokens] : traceStep.marking) {
            const auto place = explicitCpnBuilder.getPlaceIndices().find(place_id)->second;

            Colored::Multiset tokens;
            const auto& colorType = *explicitCpnBuilder.getPlaceUnderlyingColorType(place);
            for (const auto& [color, count] : traceTokens)
            {
                if (color.size() > 1)
                {
                    const auto productColorType = dynamic_cast<const Colored::ProductType*>(&colorType);
                    if (productColorType == nullptr) {
                        throw std::runtime_error("Trace color is inconsistent with underlying color type");
                    }

                    std::vector<uint32_t> colorIndices;
                    for (size_t colorTypeIndex = 0; colorTypeIndex < color.size(); colorTypeIndex++) {
                        colorIndices.push_back(
                            (*productColorType->getNestedColorType(colorTypeIndex))[color[colorTypeIndex]]->getId());
                    }

                    tokens[productColorType->getColor(colorIndices)] = count;
                }
                else
                {
                    tokens[colorType[color[0]]] = count;
                }
            }

            builder.addPlace(
                explicitCpnBuilder.getPlaceName(place),
                explicitCpnBuilder.getPlaceUnderlyingColorType(place),
                std::move(tokens),
                0,
                0);
        }

        Colored::PnmlWriter writer(builder, _traceStream);

        for (auto place = 0; place < explicitCpnBuilder.getPlaceCount(); place++)
        {
            _traceStream << "\t\t<place id=" << std::quoted(explicitCpnBuilder.getPlaceName(place)) << ">" << std::endl;
            writer.writeInitialTokens(explicitCpnBuilder.getPlaceName(place));
            _traceStream << "\t\t</place>" << std::endl;
        }

        builder.leak_colors();
    }
}

