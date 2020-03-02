#ifndef ABSTRACTDEPENDENCYGRAPH_H
#define ABSTRACTDEPENDENCYGRAPH_H

#include <cstddef>
#include <vector>
#include <cstdint>

namespace DependencyGraph {

class Configuration;
class Edge;

class BasicDependencyGraph {

public:
    virtual std::vector<Edge*> successors(Configuration *c) =0;
    virtual Configuration *initialConfiguration() =0;
    virtual void release(Edge* e) = 0;
    virtual void cleanUp() =0;
};

}
#endif // ABSTRACTDEPENDENCYGRAPH_H
