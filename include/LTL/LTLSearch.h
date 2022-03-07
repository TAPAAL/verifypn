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

/*
 * File:   LTLSearch.h
 * Author: Peter G. Jensen
 *
 * Created on 16 February 2022, 16.10
 */

#ifndef LTLSEARCH_H
#define LTLSEARCH_H

#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"
#include "LTL/Structures/BuchiAutomaton.h"
#include "LTLOptions.h"
#include "Algorithm/ModelChecker.h"
#include "Algorithm/NestedDepthFirstSearch.h"

namespace LTL {

    class LTLSearch {
    private:
        const PetriEngine::PetriNet& _net;
        const PetriEngine::PQL::Condition_ptr _query;
        Structures::BuchiAutomaton _buchi;
        PetriEngine::PQL::Condition_ptr _negated_formula;
        bool _negated_answer = false;
        APCompression _compression;
        std::unique_ptr<ModelChecker> _checker;
        std::unique_ptr<Heuristic> _heuristic;
        bool _result;

    public:
        LTLSearch(const PetriEngine::PetriNet& net,
                const PetriEngine::PQL::Condition_ptr &query, const BuchiOptimization optimization = BuchiOptimization::High,
                const APCompression compression = APCompression::Full);

        bool solve(
                const bool trace,
                const uint64_t k_bound = 0,
                const Algorithm algorithm = Algorithm::Tarjan,
                LTLPartialOrder por = LTLPartialOrder::Automaton,
                const Strategy search_strategy = Strategy::HEUR,
                const LTLHeuristic heuristics = LTLHeuristic::Automaton,
                const bool utilize_weak = true,
                const uint64_t seed = 0);
        void print_buchi(std::ostream& out, const BuchiOutType type = BuchiOutType::Dot);
        void print_stats(std::ostream& out);

        LTLPartialOrder used_partial_order() const {
            return _checker->used_partial_order();
        }

        bool is_weak() const {
            return _checker->is_weak();
        }

        std::string heuristic_type() const {
            std::stringstream ss;
            if(_heuristic)
                _heuristic->output(ss);
            return ss.str();
        }

        bool print_trace(std::ostream& out, const PetriEngine::Reducer& reducer) const;

    private:
        void _print_trace(const PetriEngine::Reducer& reducer, std::ostream& os) const;
        std::ostream &
        print_transition(size_t transition, const PetriEngine::Reducer& reducer, std::ostream &os) const;

    };

}


#endif /* LTLSEARCH_H */

