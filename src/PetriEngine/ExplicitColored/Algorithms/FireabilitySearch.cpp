#include "PetriEngine/ExplicitColored/Algorithms/FireabilitySearch.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/ExplicitColored/Visitors/FireabilityEvaluate.h"

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
        for(size_t i = 0; i < queries.size(); ++i) {
            if(results[i] == ResultPrinter::Unknown) {
                EvaluationContext ec(state.marking(), &_net);
                const auto evaluation = fireabilityEvaluate(queries[i].get(), ec);
                if(evaluation == Condition::RTRUE) {
                    // std::cout << std::endl << "Query being checked': \n";
                    // queries[0]->toString(std::cout);
                    // std::cout << std::endl;
                    const auto [res,b] = doCallback(queries[i], i, ResultPrinter::Satisfied, ss, states);
                    results[i] = res;
                    if(res == ResultPrinter::Satisfied) {
                        return true;
                    }
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
        if(r == ResultPrinter::Unknown) {
            return {ResultPrinter::Unknown,false};
        }
        // if (dynamic_cast<NotCondition*>(query.get())) {
        //     r = r == AbstractHandler::Satisfied ? AbstractHandler::NotSatisfied : AbstractHandler::Satisfied;
        // }
        if (r == ResultPrinter::Satisfied) {
            std::cout << "\nQuery: " << i << " has found a counter example" << std::endl;
            return {ResultPrinter::Satisfied,false};
            return {ResultPrinter::NotSatisfied,false};
        }
        std::cout << "\nQuery: " << i << " has not found a counter example in all reachable states" << std::endl;
        return {ResultPrinter::NotSatisfied,false};
        return {ResultPrinter::Satisfied,false};
    }
}