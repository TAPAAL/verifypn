#ifndef FIREABILITYSEARCH_H
#define FIREABILITYSEARCH_H

#include "PetriEngine/Reachability/ReachabilitySearch.h"

using namespace PetriEngine::Reachability;

namespace PetriEngine::ExplicitColored {

    class FireabilitySearch : public ReachabilitySearch {
    public:
        FireabilitySearch(PetriNet& net, AbstractHandler& callback, const int kBound = 0, const bool early = false)
            : ReachabilitySearch(net, callback, kBound, early)
        {}
        ~FireabilitySearch() = default;

        bool checkQueries(std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                              std::vector<ResultPrinter::Result>& results,
                                              Structures::State& state, searchstate_t& ss,
                                              Structures::StateSetInterface* states) override;

        std::pair<ResultPrinter::Result,bool> doCallback(
            std::shared_ptr<PQL::Condition>& query, size_t i,
            ResultPrinter::Result r,
            searchstate_t& ss,
            Structures::StateSetInterface* states) override;
    };
}
#endif //FIREABILITYSEARCH_H
