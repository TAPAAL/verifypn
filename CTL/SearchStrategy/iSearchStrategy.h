#ifndef ISEARCHSTRATEGY_H
#define ISEARCHSTRATEGY_H

#include "CTL/DependencyGraph/Edge.h"
#include <sstream>

namespace SearchStrategy {

struct Message {    
    uint32_t sender;
    uint32_t distance;
    enum Type {HALT = 0, REQUEST = 1, ANSWER_ONE = 2, ANSWER_ZERO = 3} type;
    DependencyGraph::Configuration *configuration;

    Message() {}
    Message(size_t sender, uint32_t distance, Type type, DependencyGraph::Configuration *configuration) :
           sender(sender), distance(distance), type(type), configuration(configuration) {}

    std::string ToString() {
        std::stringstream ss;
        ss << "Message from " << sender << ": ";
        ss << (type == HALT ? "Halt" : type == REQUEST ? "Request" : type == ANSWER_ONE ? "Answer 1" : "Answer 0");
        ss << configuration << "\n";
        return ss.str();
    }
};


class SearchStrategy
{
public:
    virtual bool empty() const = 0;
    virtual void pushEdge(DependencyGraph::Edge *edge) = 0;
    virtual void pushDependency(DependencyGraph::Edge* edge) = 0;
    virtual void pushNegation(DependencyGraph::Edge *edge) = 0;
    virtual DependencyGraph::Edge* popEdge(bool saturate = false) = 0;
    virtual size_t size() const = 0;
//#ifdef VERIFYPNDIST
    virtual uint32_t maxDistance() const = 0;
    virtual bool available() const = 0;   
    virtual void releaseNegationEdges(uint32_t ) = 0;
    virtual bool trivialNegation() = 0;
//#endif
};

}
#endif // SEARCHSTRATEGY_H
