#ifndef COLORED_RESULT_PRINTER_H
#define COLORED_RESULT_PRINTER_H

#include "AtomicTypes.h"
#include "PetriEngine/ExplicitColored/Algorithms/SearchStatistics.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"

namespace PetriEngine::ExplicitColored {
    struct TraceStep {
        TraceStep(
            std::string transitionId,
            std::unordered_map<std::string, std::string> binding,
            std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>> marking
        ) : transitionId(std::move(transitionId)), binding(std::move(binding)), marking(std::move(marking)), isInitial(false) {}

        TraceStep(
            std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>> marking
        ) : marking(std::move(marking)), isInitial(true) {}

        TraceStep(const TraceStep&) = default;
        TraceStep(TraceStep&&) = default;
        TraceStep& operator=(const TraceStep&) = default;
        TraceStep& operator=(TraceStep&&) = default;

        std::string transitionId;
        std::unordered_map<std::string, std::string> binding;
        std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, MarkingCount_t>>> marking;
        bool isInitial;
    };

    class IColoredResultPrinter {
    public:
        virtual void printResult(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result,
            const std::vector<TraceStep>* trace
        ) const = 0;
        virtual void printNonExplicitResult(
            std::vector<std::string> techniques,
            Reachability::AbstractHandler::Result result
        ) const = 0;
        virtual ~IColoredResultPrinter() = default;
    };

    class ColoredResultPrinter final : public IColoredResultPrinter {
    public:
        ColoredResultPrinter(
            const uint32_t queryOffset,
            std::ostream& stream,
            std::string queryName,
            const size_t seed,
            std::ostream& traceStream
        ) : _queryOffset(queryOffset), _stream(stream), _queryName(std::move(queryName)), _seed(seed), _traceStream(traceStream) {
            _techniqueFlags.emplace_back("STRUCTURAL_REDUCTION");
            _techniqueFlags.emplace_back("CPN_EXPLICIT");
        }

        void printResult(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result,
            const std::vector<TraceStep>* trace
        ) const override;

        void printNonExplicitResult(
            std::vector<std::string> techniques,
            Reachability::AbstractHandler::Result result
        ) const override;
    private:
        void _printCommon(Reachability::AbstractHandler::Result result, const std::vector<std::string>& extraTechniques) const;
        void _printTrace(const std::vector<TraceStep>& trace) const;
        uint32_t _queryOffset;
        std::ostream& _stream;
        std::string _queryName;
        size_t _seed;
        std::vector<std::string> _techniqueFlags;
        std::ostream& _traceStream;
    };
}
#endif
