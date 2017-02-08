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

    std::cout << "Initial Configuration" << std::endl;
    Configuration *v = graph->initialConfiguration();
    explore(v);

    std::cout << "Exploring" << std::endl;
    Edge *e;
    int r = strategy->pickTask(e);

//    v->printConfiguration();

    while (r != TaskType::EMPTY) {

//        std::cout << std::endl;
//        e->source->printConfiguration();
//        for (Configuration *c : e->targets) {
//            c->printConfiguration();
//        }
//        std::cout << std::endl;

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
        //std::cout << "all one " << allOne << " has czero " << hasCZero << "last: " << lastUndecided << std::endl;

        if (e->is_negated) {
            if (STATS) s_negation_edges += 1;
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
            if (STATS) s_edges += 1;
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
//        std::cout << "Picking task" << std::endl;
        r = strategy->pickTask(e);
//        std::cout << "Picked" << std::endl;
    }

    if (STATS) {
        std::cout << "[Processed edges] " << s_edges << std::endl;
        std::cout << "[Processed negation edges] " << s_negation_edges << std::endl;
        std::cout << "[Configurations explored] " << s_explored << std::endl;
        std::cout << "[Avr. succ edges per configuration] " << (((double) s_total_succ) / ((double) s_total_targets)) << std::endl;
        std::cout << "[Avr. targets per edge] " << (((double) s_total_targets) / ((double) s_total_succ)) << std::endl;
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
        s_explored += 1;
        s_total_succ += c->successors.size();
        for (auto *s : c->successors) {
            s_total_targets += s->targets.size();
        }
    }
}

void Algorithm::LocalFPA::addDependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}
