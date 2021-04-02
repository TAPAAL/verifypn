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

#include "LTL/SuccessorGeneration/Spoolers.h"
#include "LTL/SuccessorGeneration/Heuristics.h"

#include <utility>

using namespace PetriEngine::PQL;
using namespace PetriEngine;

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
    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula)
    {
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
                   std::unique_ptr<Checker> checker,
                   const options_t &options)
    {
        Result result;

/*
        std::unique_ptr<Checker> modelChecker;
        if constexpr (std::is_same_v<Checker, TarjanModelChecker<SpoolingSuccessorGenerator, true>> ||
                      std::is_same_v<Checker, TarjanModelChecker<SpoolingSuccessorGenerator, false>>) {

        } else {
            modelChecker = std::make_unique<Checker>(*net, negatedQuery, options.trace, options.ltluseweak);
        }
*/

        result.satisfied = checker->isSatisfied();
        result.is_weak = checker->isweak();
#ifdef DEBUG_EXPLORED_STATES
        result.explored_states = checker->get_explored();
#endif
        if (options.printstatistics) {
            checker->printStats(std::cout);
        }
        return result;
    }

    ReturnValue LTLMain(const PetriNet *net,
                        const Condition_ptr &query,
                        const std::string &queryName,
                        const options_t &options)
    {
        auto res = to_ltl(query);
        Condition_ptr negated_formula = res.first;
        bool negate_answer = res.second;

        bool compress = options.compress_buchi && options.buchi_out_file.empty();

        Structures::BuchiAutomaton automaton = makeBuchiAutomaton(negated_formula, compress);
        if (!options.buchi_out_file.empty()) {
            automaton.output_buchi(options.buchi_out_file, options.buchi_out_type);
        }

        Result result;
        switch (options.ltlalgorithm) {
            case Algorithm::NDFS:
                if (options.strategy == PetriEngine::Reachability::RDFS) {
                    SpoolingSuccessorGenerator gen{net, negated_formula};
                    gen.setSpooler(std::make_unique<EnabledSpooler>(net, gen));
                    gen.setHeuristic(std::make_unique<RandomHeuristic>());

                    result = _verify(net, negated_formula,
                                     std::make_unique<RandomNDFS>(net, negated_formula, automaton, std::move(gen), options.trace,
                                                                  options.ltluseweak),
                                     options);
                } else if (options.trace != TraceLevel::None) {
                    result = _verify(net, negated_formula,
                                     std::make_unique<NestedDepthFirstSearch<PetriEngine::Structures::TracableStateSet>>(
                                             net, negated_formula, automaton, options.trace, options.ltluseweak), options);
                } else {
                    result = _verify(net, negated_formula,
                                     std::make_unique<NestedDepthFirstSearch<PetriEngine::Structures::StateSet>>(
                                             net, negated_formula, automaton, options.trace, options.ltluseweak), options);
                }
                break;
            case Algorithm::Tarjan:
                if (options.strategy != PetriEngine::Reachability::DEFAULT && options.strategy != PetriEngine::Reachability::DFS) {
                    // Use spooling successor generator in case of different search strategy or stubborn set method.
                    SpoolingSuccessorGenerator gen{net, negated_formula};
                    gen.setSpooler(std::make_unique<EnabledSpooler>(net, gen));
                    if (options.strategy == PetriEngine::Reachability::RDFS) {
                        gen.setHeuristic(std::make_unique<RandomHeuristic>(options.seed_offset));
                    } else if (options.strategy == PetriEngine::Reachability::HEUR) {
                        gen.setHeuristic(std::make_unique<AutomatonHeuristic>(net, automaton));
                    }

                    if (options.trace != TraceLevel::None) {
                        result = _verify(net, negated_formula,
                                         std::make_unique<TarjanModelChecker<SpoolingSuccessorGenerator, true>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 std::move(gen),
                                                 options.trace,
                                                 options.ltluseweak),
                                         options);
                    } else {
                        result = _verify(net, negated_formula,
                                         std::make_unique<TarjanModelChecker<SpoolingSuccessorGenerator, false>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 std::move(gen),
                                                 options.trace,
                                                 options.ltluseweak),
                                         options);
                    }
                } else {
                    // no spooling needed, thus use resuming successor generation
                    if (options.trace != TraceLevel::None) {
                        result = _verify(net, negated_formula,
                                         std::make_unique<TarjanModelChecker<ResumingSuccessorGenerator, true>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 ResumingSuccessorGenerator{*net},
                                                 options.trace,
                                                 options.ltluseweak),
                                         options);
                    } else {
                        result = _verify(net, negated_formula,
                                         std::make_unique<TarjanModelChecker<ResumingSuccessorGenerator, false>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 ResumingSuccessorGenerator{*net},
                                                 options.trace,
                                                 options.ltluseweak),
                                         options);
                    }
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
                  << std::endl;
#ifdef DEBUG_EXPLORED_STATES
        std::cout << "FORMULA " << queryName << " STATS EXPLORED " << result.explored_states << std::endl;
#endif
        /*(queries[qid]->isReachability(0) ? " REACHABILITY" : "") <<*/

        return SuccessCode;
    }
}
