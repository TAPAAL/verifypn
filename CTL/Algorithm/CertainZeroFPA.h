#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"

#include <queue>

namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    virtual ~CertainZeroFPA(){}
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph,
                        SearchStrategy::DFSSearch &t_strategy) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::DFSSearch *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);

};
}
#endif // CERTAINZEROFPA_H
