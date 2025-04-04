#ifndef FIREABILITYSEARCH_H
#define FIREABILITYSEARCH_H

#include "PetriEngine/Reachability/ReachabilitySearch.h"

using namespace PetriEngine::Reachability;

namespace PetriEngine::ExplicitColored {

    class FireabilitySearch final : public ReachabilitySearch {
    public:
        FireabilitySearch(PetriNet& net, AbstractHandler& callback, const uint32_t queryTimeout)
            : ReachabilitySearch(net, callback, 0, false), _beginTime(std::chrono::high_resolution_clock::now()), _queryTimeout(queryTimeout)
        {}

        bool checkQueries(std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                              std::vector<ResultPrinter::Result>& results,
                                              Structures::State& state, searchstate_t& ss,
                                              Structures::StateSetInterface* states) override;

        std::pair<ResultPrinter::Result,bool> doCallback(
            std::shared_ptr<PQL::Condition>& query, size_t i,
            ResultPrinter::Result r,
            searchstate_t& ss,
            Structures::StateSetInterface* states) override;
    private:
        std::chrono::high_resolution_clock::time_point _beginTime;
        uint32_t _queryTimeout;

        [[nodiscard]] bool _timeout() const {
            const auto end = std::chrono::high_resolution_clock::now();
            const auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _beginTime);
            return diff.count() >= _queryTimeout;
        }
    };
}
#endif //FIREABILITYSEARCH_H
