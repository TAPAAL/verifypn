/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 * 
 * Created on March 7, 2018, 1:52 PM
 */

#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/DependencyGraph/Configuration.h"

#include <algorithm>
#include <random>

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

auto rng = std::default_random_engine {};
void RDFSSearch::flush() {
    last_parent = std::min(last_parent, W.size());
    std::shuffle(W.begin() + last_parent, W.end(), rng);
    last_parent = W.size();
}

}
