#include "CTL/Algorithm/CertainZeroFPA.h"

#include <cassert>
#include <iostream>

using namespace DependencyGraph;
using namespace SearchStrategy;

bool Algorithm::CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph &t_graph)
{
    graph = &t_graph;


    vertex = graph->initialConfiguration();
    {
        explore(vertex);
    }

    size_t cnt = 0;
    while(!strategy->empty())
    {
        while (auto e = strategy->popEdge(false)) 
        {
            ++e->refcnt;
            assert(e->refcnt >= 1);
            checkEdge(e);
            assert(e->refcnt >= -1);
            if(e->refcnt > 0) --e->refcnt;
            if(e->refcnt == 0) graph->release(e);            
            ++cnt;
            if((cnt % 1000) == 0) strategy->trivialNegation();
            if(vertex->isDone()) return vertex->assignment == ONE;
        }
        
        if(vertex->isDone()) return vertex->assignment == ONE;
        
        if(!strategy->trivialNegation())
        {
            cnt = 0;
            strategy->releaseNegationEdges(strategy->maxDistance());
            continue;
        }
    }

    return vertex->assignment == ONE;
}

void Algorithm::CertainZeroFPA::checkEdge(Edge* e, bool only_assign)
{
    if(e->handled) return;
    if(e->source->isDone())
    {
        if(e->refcnt == 0) graph->release(e);
        return;
    }
    
    bool allOne = true;
    bool hasCZero = false;
    Configuration *lastUndecided = nullptr;
    for (auto c : e->targets) {
        if (c->assignment == ONE)
        {
            continue;
        }
        else
        {
            allOne = false;

            if (c->assignment == CZERO) {
                hasCZero = true;
            }
            else if(lastUndecided == nullptr)
            {
                lastUndecided = c;
            }
        }
        if(lastUndecided  != nullptr && hasCZero) break;
    }

    if (e->is_negated) {
        _processedNegationEdges += 1;
        //Process negation edge
        if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if(only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e->source, CZERO);
            }
            if(e->refcnt == 0) { graph->release(e);}
        } else if (hasCZero) {
            finalAssign(e->source, ONE);
        } else {
            assert(lastUndecided != nullptr);
            if(only_assign) return;
            if (lastUndecided->assignment == ZERO && e->processed) {
                finalAssign(e->source, ONE);
            } else {
                if(!e->processed)
                {
                    strategy->pushNegation(e);
                }
                addDependency(e, lastUndecided);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
    } else {
        _processedEdges += 1;
        //Process hyper edge
        if (allOne) {
            finalAssign(e->source, ONE);
        } else if (hasCZero) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if(only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e->source, CZERO);
            }
            if(e->refcnt == 0) {graph->release(e);}

        } else if (lastUndecided != nullptr) {
            if(only_assign) return;
            if(!e->processed) {
                for (auto t : e->targets)
                    if(!lastUndecided->isDone())
                        addDependency(e, t);
            }                 
            if (lastUndecided->assignment == UNKNOWN) {
                explore(lastUndecided);
            }
        }
    }
    if(e->refcnt > 0  && !only_assign) e->processed = true;
    if(e->refcnt == 0) graph->release(e);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    c->nsuccs = 0;
    for (DependencyGraph::Edge *e : c->dependency_set) {
        if(e->source->isDone()) continue;
        if(!e->is_negated || a == CZERO)
        {
            strategy->pushDependency(e);
        }
        else
        {
            strategy->pushNegation(e);
        }
        assert(e->refcnt > 0);
        --e->refcnt;
        if(e->refcnt == 0) graph->release(e);
    }
    
    c->dependency_set.clear();
}

void Algorithm::CertainZeroFPA::explore(Configuration *c)
{
    
    c->assignment = ZERO;

    {
        auto succs = graph->successors(c);
        c->nsuccs = succs.size();

        _exploredConfigurations += 1;
        _numberOfEdges += c->nsuccs;
        // before we start exploring, lets check if any of them determine 
        // the outcome already!

        for(int32_t i = c->nsuccs-1; i >= 0; --i)
        {
            checkEdge(succs[i], true);
            if(c->isDone()) 
            {
                for(Edge *e : succs){
                    assert(e->refcnt <= 1);
                    if(e->refcnt >= 1) --e->refcnt;
                    if(e->refcnt == 0) graph->release(e);
                }
                return;
            }
        }

        if (c->nsuccs == 0) {
            for(Edge *e : succs){
                assert(e->refcnt <= 1);
                if(e->refcnt >= 1) --e->refcnt;
                if(e->refcnt == 0) graph->release(e);
            }
            finalAssign(c, CZERO);
            return;
        }

        for (Edge *succ : succs) {
            assert(succ->refcnt <= 1);
            if(succ->refcnt > 0)
            {
                strategy->pushEdge(succ);
                --succ->refcnt;
                if(succ->refcnt == 0) graph->release(succ);
            }
            else if(succ->refcnt == 0)
            {
                graph->release(succ);
            }
        }
    }
    strategy->flush();
}

void Algorithm::CertainZeroFPA::addDependency(Edge *e, Configuration *target)
{
    auto sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    auto tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    auto lb = std::lower_bound(std::begin(target->dependency_set), std::end(target->dependency_set), e);
    if(lb == std::end(target->dependency_set) || *lb != e)
    {
        target->dependency_set.insert(lb, e);
        assert(e->refcnt >= 0);
        ++e->refcnt;
    }
}
