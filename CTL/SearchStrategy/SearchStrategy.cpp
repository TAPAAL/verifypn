
#include <algorithm>
#include <iostream>
#include "assert.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "SearchStrategy.h"


namespace SearchStrategy{
    bool SearchStrategy::trivialNegation()
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
        return !D.empty();
    }
 
    
    void SearchStrategy::releaseNegationEdges(uint32_t dist)
    {
        for(auto it = N.begin(); it != N.end(); ++it)
        {
            assert(*it);
            if((*it)->source->getDistance() >= dist || (*it)->source->isDone())
            {
                pushEdge(*it);
                it = N.erase(it);
                if(N.empty() || it == N.end()) break;
            }
        }
    }
    
    
    uint32_t SearchStrategy::maxDistance() const
    {
        uint32_t m = 0;
        for(DependencyGraph::Edge* e : N)
        {
            if(!e->source->isDone())
                m = std::max(m, e->source->getDistance());
        }
        return m;
    }
    

    void SearchStrategy::pushDependency(DependencyGraph::Edge* edge)
    {
        if(edge->source->isDone()) return;
        edge->status = 2;
        ++edge->refcnt;
        D.push_back(edge);
    }
    
    void SearchStrategy::pushNegation(DependencyGraph::Edge* edge)
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
        }
    }

    bool SearchStrategy::empty() const
    {
        return D.empty() && N.empty() && Wsize() == 0;
    }
    
    bool SearchStrategy::available() const 
    {
        return !D.empty() || Wsize() > 0;
    }
    
    DependencyGraph::Edge* SearchStrategy::popEdge(bool saturate) {
        if(!D.empty())
        {
            auto e = D.back();
            D.pop_back();
            return e;
        }
        if(saturate)
            return nullptr;

        if(Wsize() == 0)
            return nullptr;
        return popFromW();
    }

    size_t SearchStrategy::size() const {
        return D.size() + N.size() + Wsize();
    }

    void SearchStrategy::pushEdge(DependencyGraph::Edge *edge)
    {
        if(edge->status > 0 || edge->source->isDone()) return;
        if(edge->processed && edge->is_negated)
        {
            pushNegation(edge);
            return;
        }
        edge->status = 1;
        ++edge->refcnt;
        pushToW(edge);
    }


    
}