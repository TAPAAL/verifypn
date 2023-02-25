/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:50 PM
 */

#ifndef BFSSEARCH_H
#define BFSSEARCH_H
#include <queue>
#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"

namespace SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class BFSSearch : public SearchStrategy {

protected:
    size_t Wsize() const { return W.size() + _cache.size(); };
    void pushToW(DependencyGraph::Edge* edge) {
        _cache.push_back(edge);
    }

    DependencyGraph::Edge* popFromW()
    {
        std::shuffle(_cache.begin(), _cache.end(), _rng);
        for(auto* e : _cache)
            W.emplace(e);
        _cache.clear();
        auto e = W.front();
        W.pop();
        return e;
    }

    std::queue<DependencyGraph::Edge*> W;
    std::vector<DependencyGraph::Edge*> _cache;
    std::default_random_engine _rng{};
};

}   // end SearchStrategy

#endif /* BFSSEARCH_H */

