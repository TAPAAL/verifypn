#include "CertainZeroFPA.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <iostream>

#define STATS false

using namespace DependencyGraph;

bool Algorithm::CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph &t_graph,
        SearchStrategy::iSequantialSearchStrategy &t_strategy)
{
    using namespace SearchStrategy;
    using TaskType = SearchStrategy::TaskType;

    graph = &t_graph;
    strategy = &t_strategy;

    Configuration *v = graph->initialConfiguration();
    explore(v);

    Edge *e;
    int r = strategy->pickTask(e);

    while (r != TaskType::EMPTY) {
        if (v->isDone()) {
            break;
        }

        bool allOne = true;
        bool hasCZero = false;
        Configuration *lastUndecided = nullptr;

        for (DependencyGraph::Configuration *c : e->targets) {
            if (c->assignment == CZERO) {
                hasCZero = true;
            }
            if (c->assignment != ONE) {
                allOne = false;
            }
            if (!c->isDone()) {
                lastUndecided = c;
            }
        }

        if (e->is_negated) {
            _processedNegationEdges += 1;
            //Process negation edge
            if (allOne) {
                e->source->removeSuccessor(e);
                if (e->source->successors.empty()) {
                    finalAssign(e->source, CZERO);
                }
            } else if (hasCZero) {
                finalAssign(e->source, ONE);
            } else {
                assert(lastUndecided != nullptr);
                if (lastUndecided->assignment == ZERO && e->processed) {
                    finalAssign(e->source, ONE);
                } else {
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
                e->source->removeSuccessor(e);
                if (e->source->successors.empty()) {
                    finalAssign(e->source, CZERO);
                }
            } else if (lastUndecided != nullptr) {
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

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    for (DependencyGraph::Edge *e : c->dependency_set) {
        strategy->pushDependency(e);
    }
    c->dependency_set.clear();
}

void Algorithm::CertainZeroFPA::explore(Configuration *c)
{
    c->assignment = ZERO;
    graph->successors(c);

    if (c->successors.empty()) {
        finalAssign(c, CZERO);
    }
    else {
        for (Edge *succ : c->successors) {
            strategy->pushEdge(succ);
        }
    }
    _exploredConfigurations += 1;
    _numberOfEdges += c->successors.size();
}

void Algorithm::CertainZeroFPA::addDependency(Edge *e, Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}
