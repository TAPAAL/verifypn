#include "PetriEngine/ExplicitColored/Algorithms/FireabilitySearch.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/ExplicitColored/Visitors/IgnorantFireabilityVisitor.h"

using namespace PetriEngine::Reachability;
using namespace PetriEngine::Structures;
using namespace PetriEngine::PQL;

namespace PetriEngine::ExplicitColored {
    bool FireabilitySearch::checkQueries(std::vector<std::shared_ptr<PQL::Condition > >& queries,
                                         std::vector<ResultPrinter::Result>& results,
                                         State& state, searchstate_t& ss,
                                         Structures::StateSetInterface* states)
    {
        bool allDone = true;
        if (_timeout()) {
            std::cout << "Reached timeout for fireability query reduction check" << std::endl;
            for (auto& r : results) {
                r = ResultPrinter::Result::Ignore;
            }
            return allDone;
        }
        for(size_t i = 0; i < queries.size(); ++i) {
            if(results[i] == ResultPrinter::Unknown) {
                EvaluationContext ec(state.marking(), &_net);
                const auto evaluation = fireabilityEvaluate(queries[i].get(), ec);
                if(evaluation == Condition::RTRUE) {
                    const auto [res,b] = doCallback(queries[i], i, ResultPrinter::Satisfied, ss, states);
                    results[i] = res;
                }
                else if (evaluation == Condition::RUNKNOWN) {
                    results[i] = ResultPrinter::Result::Ignore;
                }
                else {
                    allDone = false;
                }
            }
        }
        return allDone;
    }

    std::pair<ResultPrinter::Result,bool> FireabilitySearch::doCallback(
            std::shared_ptr<PQL::Condition>& query, const size_t i,
            ResultPrinter::Result r,
            searchstate_t& ss,
            Structures::StateSetInterface* states) {
        return {r, false};
    }
}