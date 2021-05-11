#ifndef WEIGHTEDAUTOMATONHEURISTIC_H_
#define WEIGHTEDAUTOMATONHEURISTIC_H_

#include "LTL/SuccessorGeneration/AutomatonHeuristic.h"

namespace LTL {
class WeightedAutomatonHeuristic : public AutomatonHeuristic {
  public:
    WeightedAutomatonHeuristic(const PetriEngine::PetriNet *net,
                               const Structures::BuchiAutomaton &aut);

    uint32_t eval(const Structures::ProductState &state, uint32_t tid) override;

    std::ostream &output(std::ostream &os) override { return os << "WEIGHTED_AUTOMATON_HEUR"; }

  private:
    std::vector<int> _bfs_dists;
};
} // namespace LTL

#endif // WEIGHTEDAUTOMATONHEURISTIC_H_
