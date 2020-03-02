/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 * 
 * Created on March 7, 2018, 1:51 PM
 */

#include "CTL/SearchStrategy/HeuristicSearch.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"

namespace SearchStrategy {

    size_t HeuristicSearch::Wsize() const {
        return W.size();
    }

    void HeuristicSearch::pushToW(DependencyGraph::Edge* edge) {
        W.push_back(edge);
    }

    DependencyGraph::Edge* HeuristicSearch::popFromW() {
        auto it = std::max_element(W.begin(), W.end(), [](auto a, auto b){
            return a->targets.size() < b->targets.size();
        });
        auto edge = *it;
        W.erase(it);
        return edge;
    }  
}
