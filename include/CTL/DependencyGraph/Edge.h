#ifndef EDGE_H
#define EDGE_H

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>

namespace DependencyGraph {

class Configuration;

class Edge {
    typedef std::vector<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    void addTarget(Configuration* conf)
    {
        assert(conf);
        targets.push_back(conf);
    }
    
    Configuration* source;
    container targets;
    uint8_t status = 0;
    bool processed = false;
    bool is_negated = false;
    int32_t refcnt = 0;
    bool handled = false;
    uint32_t weight = 0;
};
}
#endif // EDGE_H
