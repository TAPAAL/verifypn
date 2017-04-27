
#include <iostream>
#include "assert.h"
#include "DFSSearch.h"

bool SearchStrategy::DFSSearch::empty() const
{
    return W.empty();
}

void SearchStrategy::DFSSearch::pushEdge(DependencyGraph::Edge *edge)
{
    W.push_back(edge);
}

void SearchStrategy::DFSSearch::pushDependency(DependencyGraph::Edge *edge)
{
    W.push_back(edge);
}

SearchStrategy::TaskType SearchStrategy::DFSSearch::pickTask(DependencyGraph::Edge*& edge)
{
    if (W.empty()) {
        return EMPTY;
    }

    edge = W.back();

    if (!edge->is_negated || edge->processed) {
        //pop only if it's not a negation edge or it is already processed
        W.pop_back();
    }

    return EDGE;
}

unsigned int SearchStrategy::DFSSearch::maxDistance() const
{
    return 0;
}

bool SearchStrategy::DFSSearch::available() const
{
    return !W.empty();
}
