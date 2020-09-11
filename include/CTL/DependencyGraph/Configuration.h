#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "Edge.h"

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>

namespace DependencyGraph {

class Edge;

enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

class Configuration
{
    uint32_t distance = 0;
public:

    uint32_t getDistance() const { return distance; }
    void setDistance(uint32_t value) { distance = value; }

    Configuration() {}
    virtual ~Configuration();

    bool isDone() const { return assignment == ONE || assignment == CZERO; }

    uint32_t owner = 0;
    uint32_t nsuccs = 0;
    std::vector<Edge*> dependency_set;
    Assignment assignment = UNKNOWN;
};


}
#endif // CONFIGURATION_H
