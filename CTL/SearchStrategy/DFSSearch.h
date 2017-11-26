#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include <vector>
#include "CTL/DependencyGraph/Edge.h"
#include "iSearchStrategy.h"

namespace SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class DFSSearch : public SearchStrategy {

public:
    virtual ~DFSSearch(){}
    DFSSearch() {}

    bool empty() const;
    void pushEdge(DependencyGraph::Edge *edge);
    void pushDependency(DependencyGraph::Edge* edge);
    void pushNegation(DependencyGraph::Edge *edge);
    DependencyGraph::Edge* popEdge(bool saturate = false);
    size_t size() const { return W.size() + N.size() + D.size();}

    uint32_t maxDistance() const;
    bool available() const;
    void releaseNegationEdges(uint32_t );
    bool trivialNegation();
protected:
    bool possibleTrivial = false;
    std::vector<DependencyGraph::Edge*> W;
    std::vector<DependencyGraph::Edge*> N;
    std::vector<DependencyGraph::Edge*> D;
};

}   // end SearchStrategy

#endif // DFSSEARCH_H
