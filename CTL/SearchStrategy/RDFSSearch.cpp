/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 * 
 * Created on March 7, 2018, 1:52 PM
 */

#include "RDFSSearch.h"
#include "CTL/DependencyGraph/Configuration.h"

namespace SearchStrategy {
size_t RDFSSearch::Wsize() const {
    return W.size();
}

DependencyGraph::Edge* RDFSSearch::popFromW() {
    auto e = W.back();
    W.pop_back();
    last_parent = W.size();
    return e;
}

void RDFSSearch::pushToW(DependencyGraph::Edge* edge) {
    last_parent = std::min(W.size(), last_parent);
    W.push_back(edge);
}


size_t genrand(size_t i)
{
        return std::rand() % i;
}

void RDFSSearch::flush() {
    last_parent = std::min(last_parent, W.size());
    std::random_shuffle(W.begin() + last_parent, W.end(), genrand);
    last_parent = W.size();
}

}