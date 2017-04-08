#ifndef NEGATIONWAITINGLIST_H
#define NEGATIONWAITINGLIST_H

#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"
#include <deque>

namespace SearchStrategy {

class NegationWaitingList
{
    using Edge = DependencyGraph::Edge;

    std::deque<Edge*> safe_edges;
    std::vector<std::vector<Edge*>> unsafe_edges;

    // NegationWaitingList interface
public:
    bool empty() const;
    bool pop(DependencyGraph::Edge *&t);
    void push(DependencyGraph::Edge *&t);
    unsigned int maxDistance() const;
    void releaseNegationEdges(unsigned int dist);
};
}
#endif // NEGATIONWAITINGLIST_H
