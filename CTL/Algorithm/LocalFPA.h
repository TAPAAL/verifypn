#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Configuration.h"

namespace Algorithm {

class LocalFPA : public FixedPointAlgorithm
{

    // FixedPointAlgorithm interface
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph, SearchStrategy::iSequantialSearchStrategy &strategy);

protected:
    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::iSequantialSearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);

    //total number of processed edges
    long s_edges = 0;
    //total number of processed negation edges
    long s_negation_edges = 0;
    //number of explored configurations
    long s_explored = 0;
    //total number of edges found when computing successors
    long s_total_succ = 0;
    //total number of targets found when computing successors
    long s_total_targets = 0;
};
}
#endif // LOCALFPA_H
