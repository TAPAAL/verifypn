
#include <iostream>
#include "assert.h"
#include "DFSSearch.h"
#include "CTL/DependencyGraph/Configuration.h"

bool SearchStrategy::DFSSearch::empty() const
{
    return W.empty() && N.empty() && D.empty();
}

void SearchStrategy::DFSSearch::pushNegation(DependencyGraph::Edge* edge)
{
    edge->status = 3;
    ++edge->refcnt;
    bool allOne = true;
    bool hasCZero = false;

    for (DependencyGraph::Configuration *c : edge->targets) {
        if (c->assignment == DependencyGraph::Assignment::CZERO) {
            hasCZero = true;
            break;
        }
        if (c->assignment != DependencyGraph::Assignment::ONE) {
            allOne = false;
        }
    }

    if(allOne || hasCZero)
    {
        D.push_back(edge);
    }
    else
    {
        N.push_back(edge);
        possibleTrivial = true;
    }
}

void SearchStrategy::DFSSearch::pushEdge(DependencyGraph::Edge *edge)
{
    if(edge->status > 0 || edge->source->isDone()) return;
    if(edge->processed && edge->is_negated)
    {
        pushNegation(edge);
        return;
    }
    edge->status = 1;
    ++edge->refcnt;
    W.push_back(edge);
}

void SearchStrategy::DFSSearch::pushDependency(DependencyGraph::Edge* edge)
{
    if(edge->source->isDone()) return;
    edge->status = 2;
    ++edge->refcnt;
    D.push_back(edge);
}

DependencyGraph::Edge* SearchStrategy::DFSSearch::popEdge(bool saturate)
{
    if(saturate && D.empty()) return nullptr;
    
    if (W.empty() && D.empty()) {
        return nullptr;
    }

    auto edge = D.empty() ? W.back() : D.back();

    if(D.empty()) 
        W.pop_back();
    else
        D.pop_back();
    
    assert(edge->refcnt >= 0);
    --edge->refcnt;
    edge->status = 0;
    return edge;
}

uint32_t SearchStrategy::DFSSearch::maxDistance() const
{
    uint32_t m = 0;
    for(DependencyGraph::Edge* e : N)
    {
        if(!e->source->isDone())
            m = std::max(m, e->source->getDistance());
    }
    return m;
}

bool SearchStrategy::DFSSearch::available() const
{
    return !W.empty() || !D.empty();
}

void SearchStrategy::DFSSearch::releaseNegationEdges(uint32_t dist)
{
    for(auto it = N.begin(); it != N.end(); ++it)
    {
        assert(*it);
        if((*it)->source->getDistance() >= dist || (*it)->source->isDone())
        {
            W.push_back(*it);
            assert(W.front()->weight >= W.back()->weight);
            it = N.erase(it);
            if(N.empty() || it == N.end()) break;
        }
    }
}

bool SearchStrategy::DFSSearch::trivialNegation()
{
    for(auto it = N.begin(); it != N.end(); ++it)
    {
        bool allOne = true;
        bool hasCZero = false;
        auto e = *it;
        for (DependencyGraph::Configuration *c : e->targets) {
            if (c->assignment == DependencyGraph::Assignment::CZERO) {
                hasCZero = true;
                break;
            }
            if (c->assignment != DependencyGraph::Assignment::ONE) {
                allOne = false;
            }
        }
        
        if(allOne || hasCZero)
        {
            D.push_back(*it);
            it = N.erase(it);
            if(N.empty() || it == N.end()) break;
        }
    }
    possibleTrivial = false;
    return !D.empty();
}
