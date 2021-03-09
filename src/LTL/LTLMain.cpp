/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LTL/LTL.h"
#include "LTL/LTLMain.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"
#include "LTL/Algorithm/StubbornTarjanModelChecker.h"

#include <utility>

using namespace PetriEngine::PQL;

#define DEBUG_EXPLORED_STATES

namespace LTL {
    struct Result {
        bool satisfied = false;
        bool is_weak = true;
        Algorithm algorithm = Algorithm::Tarjan;
#ifdef DEBUG_EXPLORED_STATES
        size_t explored_states = 0;
#endif
    };

/**
 * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
 * In the case E f, not f is returned, and in this case the model checking result should be negated
 * (indicated by bool in return value)
 * @param formula - a formula on the form A f, E f or f
 * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
 * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
 */
    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula) {
        LTL::LTLValidator validator;
        bool should_negate = false;
        Condition_ptr converted;
        if (auto _formula = dynamic_cast<ECondition *>(formula.get())) {
            converted = std::make_shared<NotCondition>((*_formula)[0]);
            should_negate = true;
        } else if (auto _formula = dynamic_cast<ACondition *>(formula.get())) {
            converted = (*_formula)[0];
        } else {
            converted = formula;
        }
        converted->visit(validator);
        if (validator.bad()) {
            converted = nullptr;
        }
        return std::make_pair(converted, should_negate);
    }

    template<typename Checker>
    Result _verify(const PetriNet *net,
                   Condition_ptr &negatedQuery,
                   bool printstats) {
        Result result;
        auto modelChecker = std::make_unique<Checker>(*net, negatedQuery);
        result.satisfied = modelChecker->isSatisfied();
        result.is_weak = modelChecker->isweak();
#ifdef DEBUG_EXPLORED_STATES
        result.explored_states = modelChecker->get_explored();
#endif
        if (printstats) {
            modelChecker->printStats(std::cout);
        }
        return result;
    }

    ReturnValue LTLMain(const PetriNet *net,
                        const Condition_ptr &query,
                        const std::string &queryName,
                        const options_t &options) {
        auto res = to_ltl(query);
        Condition_ptr negated_formula = res.first;
        bool negate_answer = res.second;

        if (options.stubbornreduction && options.ltlalgorithm != Algorithm::Tarjan) {
            std::cerr << "Error: Stubborn reductions only supported for Tarjan's algorithm" << std::endl;
            return ErrorCode;
        }

        Result result;
        switch (options.ltlalgorithm) {
            case Algorithm::NDFS:
                result = _verify<NestedDepthFirstSearch>(net, negated_formula, options.printstatistics);
                break;
            case Algorithm::RandomNDFS:
                result = _verify<RandomNDFS>(net, negated_formula, options.printstatistics);
                break;
            case Algorithm::Tarjan:
                if (options.stubbornreduction && !negated_formula->containsNext()) {
                    std::cout << "Running stubborn version!" << std::endl;
                    result = _verify<StubbornTarjanModelChecker<ReducingSuccessorGenerator>>(net, negated_formula, options.printstatistics);
                } else {
                    result = _verify<TarjanModelChecker<false>>(net, negated_formula, options.printstatistics);
                }
                break;
            case Algorithm::None:
                assert(false);
                std::cerr << "Error: cannot LTL verify with algorithm None";
        }
        std::cout << "FORMULA " << queryName
                  << (result.satisfied ^ negate_answer ? " TRUE" : " FALSE") << " TECHNIQUES EXPLICIT "
                  << LTL::to_string(options.ltlalgorithm)
                  << (result.is_weak ? " WEAK_SKIP" : "")
                  << ((options.stubbornreduction && !negated_formula->containsNext()) ? " STUBBORN" : "")
                  << std::endl;
#ifdef DEBUG_EXPLORED_STATES
        std::cout << "FORMULA " << queryName << " STATS EXPLORED " << result.explored_states << std::endl;
#endif
        /*(queries[qid]->isReachability(0) ? " REACHABILITY" : "") <<*/

        return SuccessCode;
    }
}