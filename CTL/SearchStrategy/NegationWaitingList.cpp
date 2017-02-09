#include "NegationWaitingList.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

bool SearchStrategy::NegationWaitingList::empty() const
{
    return safe_edges.empty() && unsafe_edges.empty();
}

bool SearchStrategy::NegationWaitingList::pop(DependencyGraph::Edge *&t)
{
    if (!safe_edges.empty()) {
        t = safe_edges.front();
        safe_edges.pop_front();
        return true;
    } else {
        return false;
    }
}

void SearchStrategy::NegationWaitingList::push(DependencyGraph::Edge *&e)
{
    size_t dist = (int) e->source->getDistance();
    while (unsafe_edges.size() <= dist) {
        unsafe_edges.push_back(std::vector<Edge*>());
    }
    unsafe_edges[dist].push_back(e);
}

unsigned int SearchStrategy::NegationWaitingList::maxDistance() const
{
    return (unsigned int) std::max((int) unsafe_edges.size() - 1, 0);
}

void SearchStrategy::NegationWaitingList::releaseNegationEdges(unsigned int dist)
{
    while (unsafe_edges.size() >= dist && unsafe_edges.size() > 0) {
        while (!unsafe_edges.back().empty()) {
            Edge *e = unsafe_edges.back().back();
            unsafe_edges.back().pop_back();
            assert(e->source->getDistance() == unsafe_edges.size() - 1);
            safe_edges.push_back(e);
        }
        unsafe_edges.pop_back();
    }
}
