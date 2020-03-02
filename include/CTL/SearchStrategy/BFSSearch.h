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
    size_t Wsize() const { return W.size(); };
    void pushToW(DependencyGraph::Edge* edge) { W.push(edge); };
    DependencyGraph::Edge* popFromW() 
    {
        auto e = W.front();
        W.pop();
        return e;
    };    
    std::queue<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif /* BFSSEARCH_H */

