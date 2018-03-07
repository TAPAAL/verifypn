#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Configuration.h"

namespace Algorithm {

class LocalFPA : public FixedPointAlgorithm
{

    // FixedPointAlgorithm interface
public:
    LocalFPA(PetriEngine::Reachability::Strategy type) : FixedPointAlgorithm(type)
    {
    }
    virtual ~LocalFPA (){}
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph);

protected:
    DependencyGraph::BasicDependencyGraph *graph;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);
};
}
#endif // LOCALFPA_H
