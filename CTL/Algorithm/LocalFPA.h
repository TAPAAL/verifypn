#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Configuration.h"

namespace Algorithm {

class LocalFPA : public FixedPointAlgorithm
{

    // FixedPointAlgorithm interface
public:
    LocalFPA(PetriEngine::Reachability::Strategy type) : FixedPointAlgorithm()
    {
        if(type != PetriEngine::Reachability::DFS)
        {
            std::cerr << "CTL-Engine currently only supports DFS."   <<  std::endl;
            assert(false);
            exit(ErrorCode);
        }
        strategy = new SearchStrategy::DFSSearch();
    }
    virtual ~LocalFPA (){}
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph);

protected:
    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::SearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);
};
}
#endif // LOCALFPA_H
