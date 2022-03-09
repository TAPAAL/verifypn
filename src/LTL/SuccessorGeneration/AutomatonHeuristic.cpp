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

#include <spot/tl/formula.hh>

#include <spot/twaalgos/bfssteps.hh>

/**
 * spot::bfs_steps runs a BFS searching for states satisfying bool match(step, state).
 * This thus implements BFS_accept(q) for each NBA state q.
 */
class ReachDistance : public spot::bfs_steps {
public:
    explicit ReachDistance(spot::const_twa_graph_ptr ptr) : spot::bfs_steps(ptr) {}

private:
    bool match(spot::twa_run::step &step, const spot::state *dest) override
    {
        return std::dynamic_pointer_cast<const spot::twa_graph>(a_)->state_is_accepting(dest);
    }

    const spot::state *filter(const spot::state *s) override { return s; }
};

namespace LTL {
    AutomatonHeuristic::AutomatonHeuristic(const PetriEngine::PetriNet *net,
                                                           const Structures::BuchiAutomaton &aut)
            : _net(net), _aut(aut), _bfs_dists(aut.buchi().num_states())
    {
        _state_guards = std::move(guard_info_t::from_automaton(_aut));

        ReachDistance bfs_calc(_aut.buchi_ptr());
        for (unsigned state = 0; state < _aut.buchi().num_states(); ++state) {
            if (_aut.buchi().state_is_accepting(state)) {
                _bfs_dists[state] = 1;
            } else {
                spot::twa_run::steps steps;
                // Calculate BFS distance to accepting state.
                bfs_calc.search(_aut.buchi().state_from_number(state), steps);
                _bfs_dists[state] = steps.size() + 1;
            }
        }
    }

    uint32_t AutomatonHeuristic::eval(const Structures::ProductState &state, uint32_t)
    {
        assert(state.get_buchi_state() < _state_guards.size());
        const auto &guardInfo = _state_guards[state.get_buchi_state()];
        if (guardInfo._is_accepting)
            return 0;
        uint32_t min_dist = std::numeric_limits<uint32_t>::max();
        PetriEngine::PQL::DistanceContext context{_net, state.marking()};
        for (const auto& guard : guardInfo._progressing) {
            uint32_t dist = _bfs_dists[guard._dest] * guard._condition->distance(context);
            if (dist < min_dist)
                min_dist = dist;
        }
        return min_dist;
    }

    bool AutomatonHeuristic::has_heuristic(const Structures::ProductState &state)
    {
        assert(state.get_buchi_state() < _state_guards.size());
        return !_state_guards[state.get_buchi_state()]._is_accepting;
    }
} // namespace LTL

