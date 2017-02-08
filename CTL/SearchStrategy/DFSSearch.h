#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include "iSearchStrategy.h"

#include <vector>

namespace SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class DFSSearch : public iSequantialSearchStrategy, public iDistributedSearchStrategy
{
public:
    DFSSearch() {}

    virtual bool empty() const override;
    virtual void pushEdge(DependencyGraph::Edge *edge) override;
    virtual void pushDependency(DependencyGraph::Edge *edge) override;
    virtual TaskType pickTask(DependencyGraph::Edge*& edge) override;

    virtual unsigned int maxDistance() const override;
    virtual bool available() const override;
    virtual void pushMessage(Message &message) override;
    virtual void releaseNegationEdges(int dist) override;

    virtual TaskType pickTask(DependencyGraph::Edge*& edge,
                              Message& message) override;


protected:

    std::vector<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif // DFSSEARCH_H
