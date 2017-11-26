#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"


namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    CertainZeroFPA(PetriEngine::Reachability::Strategy type) : FixedPointAlgorithm()
    {
        if(type != PetriEngine::Reachability::DFS)
        {
            std::cerr << "CTL-Engine currently only supports DFS."   <<  std::endl;
            exit(ErrorCode);
        }
        strategy = new SearchStrategy::DFSSearch();
    }
    virtual ~CertainZeroFPA()
    {
        delete strategy;
    }
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::DFSSearch *strategy;
    DependencyGraph::Configuration* vertex;
    
    void checkEdge(DependencyGraph::Edge* e, bool only_assign = false);
    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);

};
}
#endif // CERTAINZEROFPA_H
