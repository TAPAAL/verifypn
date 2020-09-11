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
    virtual ~SearchStrategy() {};
    bool empty() const;
    void pushEdge(DependencyGraph::Edge *edge);
    void pushDependency(DependencyGraph::Edge* edge);
    void pushNegation(DependencyGraph::Edge *edge);
    DependencyGraph::Edge* popEdge(bool saturate = false);
    size_t size() const;
//#ifdef VERIFYPNDIST
    uint32_t maxDistance() const;
    bool available() const;
    void releaseNegationEdges(uint32_t );
    bool trivialNegation();
    virtual void flush() {};
//#endif
protected:
    virtual size_t Wsize() const = 0;
    virtual void pushToW(DependencyGraph::Edge* edge) = 0;
    virtual DependencyGraph::Edge* popFromW() = 0;
    
    std::vector<DependencyGraph::Edge*> N;
    std::vector<DependencyGraph::Edge*> D;
};

}
#endif // SEARCHSTRATEGY_H
