/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:51 PM
 */

#ifndef HEURISTICSEARCH_H
#define HEURISTICSEARCH_H

#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"

#include <vector>
#include <queue>

namespace SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

    struct hcomp_t {
        bool operator() (const DependencyGraph::Edge* a, const DependencyGraph::Edge* b) const;
    };

class HeuristicSearch : public SearchStrategy {

protected:
    size_t Wsize() const;
    void pushToW(DependencyGraph::Edge* edge);
    DependencyGraph::Edge* popFromW();
    std::priority_queue<DependencyGraph::Edge*, std::vector<DependencyGraph::Edge*>, hcomp_t> W;

};



}   // end SearchStrategy

#endif /* HEURISTICSEARCH_H */

