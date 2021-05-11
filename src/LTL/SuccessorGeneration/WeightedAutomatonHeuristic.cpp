#include "LTL/SuccessorGeneration/WeightedAutomatonHeuristic.h"
#include "LTL/Simplification/SpotToPQL.h"

#include <spot/tl/formula.hh>
#include <spot/twa/formula2bdd.hh>

#include <spot/twaalgos/bfssteps.hh>

class reach_distance : public spot::bfs_steps {
  public:
    reach_distance(const spot::const_twa_graph_ptr &ptr) : spot::bfs_steps(ptr) {}

  private:
    bool match(spot::twa_run::step &step, const spot::state *dest) override
    {
        return std::dynamic_pointer_cast<const spot::twa_graph>(a_)->state_is_accepting(dest);
    }

    const spot::state *filter(const spot::state *s) override { return s; }
};

namespace LTL {
WeightedAutomatonHeuristic::WeightedAutomatonHeuristic(const PetriEngine::PetriNet *net,
                                                       const Structures::BuchiAutomaton &aut)
    : AutomatonHeuristic(net, aut), _bfs_dists(aut._buchi->num_states())
{
    reach_distance bfs_calc{_aut._buchi};
    for (unsigned state = 0; state < _aut._buchi->num_states(); ++state) {
        if (_aut._buchi->state_is_accepting(state)) {
            _bfs_dists[state] = 1;
        }
        else {
            spot::twa_run::steps steps;
            bfs_calc.search(_aut._buchi->state_from_number(state), steps);
            _bfs_dists[state] = steps.size() + 1;
        }
    }
}

uint32_t WeightedAutomatonHeuristic::eval(const Structures::ProductState &state, uint32_t)
{
    assert(state.getBuchiState() < _state_guards.size());
    const auto &guardInfo = _state_guards[state.getBuchiState()];
    if (guardInfo.is_accepting)
        return 0;
    uint32_t min_dist = std::numeric_limits<uint32_t>::max();
    PetriEngine::PQL::DistanceContext context{_net, state.marking()};
    for (const auto guard : guardInfo.progressing) {
        uint32_t dist = _bfs_dists[guard.dest] * guard.condition->distance(context);
        if (dist < min_dist)
            min_dist = dist;
    }
    return min_dist;
}
} // namespace LTL
