#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "../DependencyGraph/BasicDependencyGraph.h"
#include "../SearchStrategy/iSearchStrategy.h"
//#include "../Communicator/Communicator.h"
//#include "PartitionFunction.h"

namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iSequantialSearchStrategy &strategy) =0;

protected:
    //total number of processed edges
    size_t s_edges = 0;
    //total number of processed negation edges
    size_t s_negation_edges = 0;
    //number of explored configurations
    size_t s_explored = 0;
    //total number of edges found when computing successors
    size_t s_total_succ = 0;
    //total number of targets found when computing successors
    size_t s_total_targets = 0;
};

//class DistributedFixedPointAlgorithm {
//public:
//    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
//                        SearchStrategy::iDistributedSearchStrategy &strategy,
//                        Communicator &communicator,
//                        PartitionFunction &partition_function) =0;
//};

}
#endif // FIXEDPOINTALGORITHM_H
