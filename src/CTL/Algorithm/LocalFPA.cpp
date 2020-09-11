#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"

#include <cassert>
#include <iostream>

bool Algorithm::LocalFPA::search(DependencyGraph::BasicDependencyGraph &t_graph)
{
    using namespace DependencyGraph;
    graph = &t_graph;

    Configuration *v = graph->initialConfiguration();
    explore(v);

    while (!strategy->empty())
    {
        while (auto e = strategy->popEdge()) {

            if (v->assignment == DependencyGraph::ONE) {
                break;
            }

            bool allOne = true;
            Configuration *lastUndecided = nullptr;

            for (DependencyGraph::Configuration *c : e->targets) {
                if (c->assignment != DependencyGraph::ONE) {
                    allOne = false;
                    lastUndecided = c;
                }
            }

            if (e->is_negated) {
                _processedNegationEdges += 1;
                //Process negation edge
                if(allOne) {}
                else if(!e->processed){
                    addDependency(e, lastUndecided);
                    if(lastUndecided->assignment == UNKNOWN){
                        explore(lastUndecided);
                    }
                    strategy->pushNegation(e);
                }
                else{
                    finalAssign(e->source, ONE);
                }

            } else {
                _processedEdges += 1;
                //Process hyper edge
                if (allOne) {
                    finalAssign(e->source, ONE);
                } else {
                    addDependency(e, lastUndecided);
                    if (lastUndecided->assignment == UNKNOWN) {
                        explore(lastUndecided);
                    }
                }
            }
            e->processed = true;
            if(e->refcnt == 0) graph->release(e);
        }
        if(!strategy->trivialNegation())
        {
            strategy->releaseNegationEdges(strategy->maxDistance());
        }
    }

    return v->assignment == ONE;
}

void Algorithm::LocalFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == DependencyGraph::ONE);
    c->assignment = a;

    for(DependencyGraph::Edge *e : c->dependency_set){
        if(e->is_negated)
        {
            strategy->pushNegation(e);
        }
        else
        {
            strategy->pushDependency(e);
        }
        --e->refcnt;
        if(e->refcnt == 0) graph->release(e);
    }

    c->dependency_set.clear();
}

void Algorithm::LocalFPA::explore(DependencyGraph::Configuration *c)
{
    assert(c->assignment == DependencyGraph::UNKNOWN);
    c->assignment = DependencyGraph::ZERO;
    auto succs = graph->successors(c);

    for (DependencyGraph::Edge *succ : succs) {
        strategy->pushEdge(succ);
        --succ->refcnt;
        if(succ->refcnt == 0) graph->release(succ);
    }

    _exploredConfigurations += 1;
    _numberOfEdges += succs.size();
}

void Algorithm::LocalFPA::addDependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));

    auto lb = std::lower_bound(std::begin(target->dependency_set), std::end(target->dependency_set), e);
    if(lb == std::end(target->dependency_set) || *lb != e)
    {
        target->dependency_set.insert(lb, e);
        ++e->refcnt;
    }
}
