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
enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

class Edge {
    typedef std::forward_list<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    void addTarget(Configuration* conf)
    {
        assert(conf);
        targets.push_front(conf);
        ++children;
    }
    
    container targets;    
    Configuration* source;
    uint8_t status = 0;
    bool processed = false;
    bool is_negated = false;
    bool handled = false;
    int32_t refcnt = 0;
    size_t children;
    Assignment assignment;
};
}
#endif // EDGE_H
