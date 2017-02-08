#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"
#include "../SearchStrategy/iSearchStrategy.h"

#include <queue>

namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
    struct edge_prioritizer {
        bool operator()(DependencyGraph::Edge* const &l,
                        DependencyGraph::Edge* const &r) const {
            return l->source->getDistance() > r->source->getDistance();
        }
    };
    using Edge = DependencyGraph::Edge;
    using QueueContainer = std::vector<Edge*>;
    using Queue = std::priority_queue<Edge*, QueueContainer, edge_prioritizer>;

public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph,
                        SearchStrategy::iSequantialSearchStrategy &t_strategy) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::iSequantialSearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);

};
}
#endif // CERTAINZEROFPA_H
