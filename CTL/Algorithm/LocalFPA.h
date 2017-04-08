#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Configuration.h"

namespace Algorithm {

class LocalFPA : public FixedPointAlgorithm
{

    // FixedPointAlgorithm interface
public:
    virtual ~LocalFPA (){}
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph, SearchStrategy::iSequantialSearchStrategy &strategy);

protected:
    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::iSequantialSearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);
};
}
#endif // LOCALFPA_H
