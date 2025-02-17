#ifndef COLORED_RESULT_PRINTER_H
#define COLORED_RESULT_PRINTER_H

#include "PetriEngine/ExplicitColored/SearchStatistics.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"

namespace PetriEngine::ExplicitColored {
    class IColoredResultPrinter {
    public:
        virtual void printResults(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result
        ) const = 0;
        virtual ~IColoredResultPrinter() = default;
    };

    class ColoredResultPrinter final : public IColoredResultPrinter {
    public:
        ColoredResultPrinter(
            const uint32_t queryOffset,
            std::ostream& stream,
            std::vector<std::string> queryNames,
            const size_t seed
        ) : _queryOffset(queryOffset), _stream(stream), _queryNames(std::move(queryNames)), _seed(seed) {
            _techniqueFlags.push_back("STRUCTURAL_REDUCTION");
            _techniqueFlags.push_back("CPN_EXPLICIT");
        }

        void printResults(
            const SearchStatistics& searchStatistics,
            Reachability::AbstractHandler::Result result
        ) const override;
    private:
        uint32_t _queryOffset;
        std::ostream& _stream;
        std::vector<std::string> _queryNames;
        size_t _seed;
        std::vector<std::string> _techniqueFlags;
    };
}
#endif
