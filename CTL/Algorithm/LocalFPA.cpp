#include "LocalFPA.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"

#include <assert.h>
#include <iostream>

#define STATS false

bool Algorithm::LocalFPA::search(DependencyGraph::BasicDependencyGraph &t_graph,
                                 SearchStrategy::iSequantialSearchStrategy &t_strategy)
{
    using namespace DependencyGraph;
    using TaskType = SearchStrategy::TaskType;

    graph = &t_graph;
    strategy = &t_strategy;

    Configuration *v = graph->initialConfiguration();
    explore(v);

    Edge *e;
    int r = strategy->pickTask(e);

    while (r != TaskType::EMPTY) {

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
        r = strategy->pickTask(e);
    }

    return (v->assignment == ONE) ? true : false;
}

void Algorithm::LocalFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == DependencyGraph::ONE);
    c->assignment = a;

    for(DependencyGraph::Edge *e : c->dependency_set){
        strategy->pushEdge(e);
    }

    c->dependency_set.clear();
}

void Algorithm::LocalFPA::explore(DependencyGraph::Configuration *c)
{
    assert(c->assignment == DependencyGraph::UNKNOWN);
    c->assignment = DependencyGraph::ZERO;
    graph->successors(c);

    for (DependencyGraph::Edge *succ : c->successors) {
        strategy->pushEdge(succ);
    }

    if (STATS) {
        _exploredConfigurations += 1;
        _numberOfEdges += c->successors.size();
    }
}

void Algorithm::LocalFPA::addDependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}
