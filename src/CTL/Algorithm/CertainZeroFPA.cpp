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

    /*{
        bool any = false;
        size_t n = 0;
        size_t k = 0;
        for(Edge* deps : e->source->dependency_set)
        {
            ++k;
            if(deps->source->isDone()) continue;
            any = true;
            ++n;
        }
        if(!any && e->source != vertex) return;
    }*/

    bool allOne = true;
    bool hasCZero = false;
    //auto pre_empty = e->targets.empty();
    Configuration *lastUndecided = nullptr;
    {
        auto it = e->targets.begin();
        auto pit = e->targets.before_begin();
        while(it != e->targets.end())
        {
            if ((*it)->assignment == ONE)
            {
                e->targets.erase_after(pit);
                it = pit;
            }
            else
            {
                allOne = false;
                if ((*it)->assignment == CZERO) {
                    hasCZero = true;
                    //assert(e->assignment == CZERO || only_assign);
                    break;
                }
                else if(lastUndecided == nullptr)
                {
                    lastUndecided = *it;
                }
                else if(lastUndecided != nullptr && lastUndecided->assignment == UNKNOWN && (*it)->assignment == ZERO)
                {
                    lastUndecided = *it;
                }
            }
            pit = it;
            ++it;
        }
    }
    /*if(e->targets.empty())
    {
        assert(e->assignment == ONE || e->children == 0);
    }*/

    if (e->is_negated) {
        _processedNegationEdges += 1;
        //Process negation edge
        if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if(only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e, CZERO);
            }
            if(e->refcnt == 0) { graph->release(e);}
        } else if (hasCZero) {
            finalAssign(e, ONE);
        } else {
            assert(lastUndecided != nullptr);
            if(only_assign) return;
            if (lastUndecided->assignment == ZERO && e->processed) {
                finalAssign(e, ONE);
            } else {
                if(!e->processed)
                {
                    strategy->pushNegation(e);
                }
                lastUndecided->addDependency(e);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
    } else {
        _processedEdges += 1;
        //Process hyper edge
        if (allOne) {
            finalAssign(e, ONE);
        } else if (hasCZero) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if(only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e, CZERO);
            }
            if(e->refcnt == 0) {graph->release(e);}

        } else if (lastUndecided != nullptr) {
            if(only_assign) return;
            if(!e->processed) {
                if(!lastUndecided->isDone())
                {
                    for (auto t : e->targets)
                        t->addDependency(e);
                }
            }
            if (lastUndecided->assignment == UNKNOWN) {
                explore(lastUndecided);
            }
        }
    }
    if(e->refcnt > 0  && !only_assign) e->processed = true;
    if(e->refcnt == 0) graph->release(e);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a)
{
    finalAssign(e->source, a);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    c->nsuccs = 0;
    for (DependencyGraph::Edge *e : c->dependency_set) {
        if(!e->source->isDone()) {
            if(a == CZERO)
            {
                /*e->assignment = CZERO;*/
            }
            else if(a == ONE)
            {
                /*assert(e->children >= 1);
                --e->children;
                if(e->children == 0)
                    e->assignment = ONE;*/
            }
            if(!e->is_negated || a == CZERO)
            {
                strategy->pushDependency(e);
            }
            else
            {
                strategy->pushNegation(e);
            }
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
