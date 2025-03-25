#ifndef EXPLICITCOLOREDMODELCHECKER_H
#define EXPLICITCOLOREDMODELCHECKER_H
#include "PetriEngine/options.h"
#include "PetriEngine/PQL/PQL.h"
#include "utils/structures/shared_string.h"
#include <sstream>
#include "Visitors/ConditionCopyVisitor.h"

#include "ColoredResultPrinter.h"

namespace PetriEngine::ExplicitColored {
    class ExplicitColoredModelChecker {
    public:
        enum class Result {
            SATISFIED,
            UNSATISFIED,
            UNKNOWN
        };

        explicit ExplicitColoredModelChecker(shared_string_set& stringSet, std::ostream& fullStatisticOut)
            : _stringSet(stringSet), _fullStatisticOut(fullStatisticOut)
            {}

        Result checkQuery(
            const std::string& modelPath,
            const PQL::Condition_ptr& query,
            options_t& options,
            IColoredResultPrinter* resultPrinter
        ) const;
    private:
        Result checkColorIgnorantLP(
            const std::string& pnmlModel,
            const PQL::Condition_ptr& query,
            options_t& options
        ) const;

        Result explicitColorCheck(
            const std::string& pnmlModel,
            const PQL::Condition_ptr& query,
            options_t& options,
            SearchStatistics* searchStatistics
        ) const;

        void _reduce(
            const std::string& pnmlModel,
            std::stringstream& out,
            const PQL::Condition_ptr& query,
            options_t& options
        ) const;
        shared_string_set& _stringSet;
        std::ostream& _fullStatisticOut;
    };
}
#endif //EXPLICITCOLOREDMODELCHECKER_H
