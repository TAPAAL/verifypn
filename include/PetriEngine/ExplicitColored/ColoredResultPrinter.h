#ifndef COLORED_RESULT_PRINTER_H
#define COLORED_RESULT_PRINTER_H

#include "PetriEngine/ExplicitColored/SearchStatistics.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"

namespace PetriEngine::ExplicitColored {
    class IColoredResultPrinter {
    public:
        virtual void printResult(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result
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
            const size_t seed
        ) : _queryOffset(queryOffset), _stream(stream), _queryName(std::move(queryName)), _seed(seed) {
            _techniqueFlags.push_back("STRUCTURAL_REDUCTION");
            _techniqueFlags.push_back("CPN_EXPLICIT");
        }

        void printResult(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result
        ) const override;

        void printNonExplicitResult(
            std::vector<std::string> techniques,
            Reachability::AbstractHandler::Result result
        ) const override;
    private:

        void _printCommon(Reachability::AbstractHandler::Result result, const std::vector<std::string>& extraTechniques) const;
        uint32_t _queryOffset;
        std::ostream& _stream;
        std::string _queryName;
        size_t _seed;
        std::vector<std::string> _techniqueFlags;
    };
}
#endif
