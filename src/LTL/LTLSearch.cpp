/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>,
 *                     Peter G. Jensen <root@petergjoel.dk>
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

#include "LTL/LTLSearch.h"
#include "LTL/LTLValidator.h"
#include "LTL/SuccessorGeneration/Spoolers.h"
#include "LTL/SuccessorGeneration/Heuristics.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"
#include "LTL/Algorithm/NestedDepthFirstSearch.h"
#include "LTL/Algorithm/TarjanModelChecker.h"

#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/options.h"

#include <utility>

using namespace PetriEngine::PQL;
using namespace PetriEngine;


namespace LTL {

    /**
     * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
     * In the case E f, not f is returned, and in this case the model checking result should be negated
     * (indicated by bool in return value)
     * @param formula - a formula on the form A f, E f or f
     * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
     * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
     */
    std::tuple<Condition_ptr, bool> to_ltl(const Condition_ptr &formula) {
        LTL::LTLValidator validator;
        bool should_negate = false;
        Condition_ptr converted;
        if (auto _formula = dynamic_cast<ECondition *> (formula.get())) {
            converted = std::make_shared<NotCondition>((*_formula)[0]);
            should_negate = true;
        } else if (auto _formula = dynamic_cast<ACondition *> (formula.get())) {
            converted = (*_formula)[0];
        } else if (auto _formula = dynamic_cast<AGCondition *> (formula.get())) {
            auto f = std::make_shared<ACondition>(std::make_shared<GCondition>((*_formula)[0]));
            return to_ltl(f);
        } else if (auto _formula = dynamic_cast<AFCondition *> (formula.get())) {
            auto f = std::make_shared<ACondition>(std::make_shared<FCondition>((*_formula)[0]));
            return to_ltl(f);
        }
        else if (auto _formula = dynamic_cast<EFCondition *> (formula.get())) {
            auto f = std::make_shared<ECondition>(std::make_shared<FCondition>((*_formula)[0]));
            return to_ltl(f);
        }
        else if (auto _formula = dynamic_cast<EGCondition *> (formula.get())) {
            auto f = std::make_shared<ECondition>(std::make_shared<GCondition>((*_formula)[0]));
            return to_ltl(f);
        }
        else if (auto _formula = dynamic_cast<AUCondition *> (formula.get())) {
            auto f = std::make_shared<ACondition>(std::make_shared<UntilCondition>((*_formula)[0], (*_formula)[1]));
            return to_ltl(f);
        }
        else if (auto _formula = dynamic_cast<EUCondition *> (formula.get())) {
            auto f = std::make_shared<ECondition>(std::make_shared<UntilCondition>((*_formula)[0], (*_formula)[1]));
            return to_ltl(f);
        }
        else {
            converted = formula;
        }
        Visitor::visit(validator, converted);
        if (validator.bad()) {
            converted = nullptr;
        }
        return std::make_pair(converted, should_negate);
    }

    std::unique_ptr<Heuristic> make_heuristic(const PetriNet& net,
        const Condition_ptr &negated_formula,
        const Structures::BuchiAutomaton& automaton,
        const Strategy search_strategy = Strategy::HEUR,
        const LTLHeuristic heuristics = LTLHeuristic::Automaton,
        const uint64_t seed = 0) {
        if (search_strategy == Strategy::RDFS || heuristics == LTLHeuristic::RDFS) {
            return std::make_unique<RandomHeuristic>(seed);
        }
        if (search_strategy != Strategy::HEUR && search_strategy != Strategy::DEFAULT) {
            return nullptr;
        }
        switch (heuristics) {
            case LTLHeuristic::Distance:
                return std::make_unique<DistanceHeuristic>(&net, negated_formula);
            case LTLHeuristic::Automaton:
                return std::make_unique<AutomatonHeuristic>(&net, automaton);
            case LTLHeuristic::FireCount:
                return std::make_unique<LogFireCountHeuristic>(net.numberOfTransitions(), 5000);
            case LTLHeuristic::DFS:
            case LTLHeuristic::RDFS:
                return nullptr;
            default:
                throw base_error("Unknown LTL heuristics: ", to_underlying(heuristics));
        }
    }

    LTLSearch::LTLSearch(const PetriEngine::PetriNet& net,
        const PetriEngine::PQL::Condition_ptr &query, const BuchiOptimization optimization, const APCompression compression)
    : _net(net), _query(query), _compression(compression) {
        std::tie(_negated_formula, _negated_answer) = to_ltl(query);
        _buchi = make_buchi_automaton(_negated_formula, optimization, compression);
    }

    void LTLSearch::print_buchi(std::ostream& out, const BuchiOutType type)
    {
        if(_compression != APCompression::None)
            throw base_error("Printing of Büchi automata only supported with APCompression::None");
        _buchi.output_buchi(out, type);
    }

    void LTLSearch::print_stats(std::ostream& out)
    {
        _checker->print_stats(out);
    }

    static constexpr auto _indent = "  ";
    static constexpr auto _tokenIndent = "    ";

    void print_loop(std::ostream &os) {
        os << _indent << "<loop/>\n";
    }

    // TODO refactor this into a trace-printer, this does not belong in the solver.
    void LTLSearch::_print_trace(const PetriEngine::Reducer& reducer, std::ostream& os) const {
        os << "<trace>\n";
        reducer.initFire(os);
        auto& trace = _checker->trace();
        for (size_t i = 0; i < trace.size(); ++i) {
            if (i == _checker->loop_index())
                print_loop(os);
            print_transition(trace[i], reducer, os);
        }
        os << std::endl << "</trace>" << std::endl;
    }

    std::ostream &
    LTLSearch::print_transition(size_t transition, const PetriEngine::Reducer& reducer, std::ostream &os) const {
        if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
            os << _indent << "<deadlock/>";
            return os;
        }
        os << _indent << "<transition id="
                // field width stuff obsolete without büchi state printing.
                << std::quoted(*_net.transitionNames()[transition]);
        os << ">";
        reducer.extraConsume(os, *_net.transitionNames()[transition]);
        os << std::endl;
        auto [fpre, lpre] = _net.preset(transition);
        for (; fpre < lpre; ++fpre) {
            if (fpre->inhibitor) {
                continue;
            }
            for (size_t i = 0; i < fpre->tokens; ++i) {
                os << _tokenIndent << R"(<token age="0" place=")" << *_net.placeNames()[fpre->place] << "\"/>\n";
            }
        }
        os << _indent << "</transition>\n";
        reducer.postFire(os, *_net.transitionNames()[transition]);
        return os;
    }

    bool LTLSearch::print_trace(std::ostream& out, const PetriEngine::Reducer& reducer) const
    {
        if(_result)
        {
            _print_trace(reducer, out);
            return true;
        }
        else
            return false;
    }

    bool LTLSearch::solve(  const bool trace,
                            const uint64_t k_bound,
                            const Algorithm algorithm,
                            const LTL::LTLPartialOrder por,
                            const Strategy search_strategy,
                            const LTLHeuristic heuristics_flag,
                            const bool utilize_weak,
                            const uint64_t seed) {

        _heuristic = make_heuristic(_net, _negated_formula, _buchi, search_strategy, heuristics_flag, seed);

        switch (algorithm) {
            case Algorithm::NDFS:
            {
                _checker = std::make_unique<NestedDepthFirstSearch>(_net, _negated_formula, _buchi, k_bound);
                break;
            }
            case Algorithm::Tarjan:
                _checker = std::make_unique<TarjanModelChecker>(_net, _negated_formula, _buchi, k_bound);
                break;
            case Algorithm::None:
            default:
                assert(false);
                std::cerr << "Error: cannot LTL verify with algorithm None";
        }
        _checker->set_utilize_weak(utilize_weak);
        _checker->set_heuristic(_heuristic.get());
        _checker->set_partial_order(por);
        _result = _checker->check();
        return _result xor _negated_answer;
    }
}