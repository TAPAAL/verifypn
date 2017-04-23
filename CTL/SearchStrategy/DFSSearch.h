#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include <vector>
#include "../DependencyGraph/Edge.h"

namespace SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

enum TaskType {
    EMPTY = 0,
    EDGE = 1,
};

class DFSSearch {

public:
    virtual ~DFSSearch(){}
    DFSSearch() {}

    bool empty() const;
    void pushEdge(DependencyGraph::Edge *edge);
    void pushDependency(DependencyGraph::Edge *edge);
    TaskType pickTask(DependencyGraph::Edge*& edge);

    unsigned int maxDistance() const;
    bool available() const;
    void releaseNegationEdges(int dist);

protected:

    std::vector<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif // DFSSEARCH_H
