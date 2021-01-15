#ifndef EDGE_H
#define EDGE_H

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <forward_list>

namespace DependencyGraph {

class Configuration;

class Edge {
    typedef std::forward_list<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    void addTarget(Configuration* conf)
    {
        assert(conf);
        targets.push_front(conf);
    }
    
    container targets;    
    Configuration* source;
    uint8_t status = 0;
    bool processed = false;
    bool is_negated = false;
    bool handled = false;
    int32_t refcnt = 0;
    uint32_t weight = 0;
};
}
#endif // EDGE_H
