/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "LTL/SuccessorGeneration/AutomatonHeuristic.h"
#include "LTL/Simplification/SpotToPQL.h"

#include <spot/twa/formula2bdd.hh>
#include <spot/tl/formula.hh>


namespace LTL {
    AutomatonHeuristic::AutomatonHeuristic(const PetriEngine::PetriNet *net, const Structures::BuchiAutomaton &aut)
            : _net(net), _aut(aut)
    {
        std::vector<AtomicProposition> aps(_aut.ap_info.size());
        std::transform(std::begin(_aut.ap_info), std::end(_aut.ap_info), std::begin(aps),
                       [](const std::pair<int, AtomicProposition> &pair) { return pair.second; });
        for (unsigned state = 0; state < _aut._buchi->num_states(); ++state) {
            _state_guards.emplace_back(state, _aut._buchi->state_is_accepting(state));
            for (auto &e : _aut._buchi->out(state)) {
                auto formula = spot::bdd_to_formula(e.cond, _aut.dict);
                if (e.dst == state) {
                    _state_guards.back().retarding = toPQL(formula, aps);
                } else {
                    _state_guards.back().progressing.push_back(toPQL(formula, aps));
                }
            }
            if (!_state_guards.back().retarding) {
                _state_guards.back().retarding = std::make_shared<BooleanCondition>(false);
            }
        }
    }

    bool AutomatonHeuristic::has_heuristic(const Structures::ProductState &state)
    {
        assert(state.getBuchiState() < _state_guards.size());
        return !_state_guards[state.getBuchiState()].is_accepting;
    }

    uint32_t AutomatonHeuristic::eval(const Structures::ProductState &state, uint32_t)
    {
        assert(state.getBuchiState() < _state_guards.size());
        const auto &guardInfo = _state_guards[state.getBuchiState()];
        if (guardInfo.is_accepting) return 0;
        uint32_t min_dist = std::numeric_limits<uint32_t>::max();
        PetriEngine::PQL::DistanceContext context{_net, state.marking()};
        for (const auto &condition : guardInfo.progressing) {
            uint32_t dist = condition->distance(context);
            if (dist < min_dist) min_dist = dist;
        }
        return min_dist;
    }
}