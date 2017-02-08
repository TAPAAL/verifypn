#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "iSearchStrategy.h"
#include "NegationWaitingList.h"
#include "WaitingList.h"
#include "TypeMessageList.h"

#include <assert.h>

#include <stack>

namespace SearchStrategy {

using Edge = DependencyGraph::Edge;

//Template arguments:
//EdgeList implements a WaitingList interface, used to store pointers to edge pointers.
//NegationList implements a NegationWaitingList interface, used to store poitners to edge pointers.
//MessageList implements a WaitingList interface, used to store message structs.
//DependencyList implements a WaitingList interface, used to store edge pointers.

template<
        class EdgeList = WaitingList<Edge*, std::stack<Edge*>>,
        class NegationList = NegationWaitingList,
        class MessageList = TypeMessageList,  //WaitingList<Message, std::queue<Message>>
        class DependencyList = WaitingList<Edge*, std::stack<Edge*>>
>
class UniversalSearchStrategy : public iSequantialSearchStrategy, public iDistributedSearchStrategy {

public:
    UniversalSearchStrategy(
            EdgeList edge_list = EdgeList(),
            NegationList negation_edge_list = NegationList(),
            MessageList message_list = MessageList(),
            DependencyList dependency_list = DependencyList()
    ) : W(edge_list), N(negation_edge_list), M(message_list), D(dependency_list) {}


    virtual bool empty() const override {
        return W.empty() && D.empty() && M.empty() && N.empty();
    }

    virtual unsigned int maxDistance() const override {
        return N.maxDistance();
    }

    virtual bool available() const override {
        return !W.empty();
    }

    virtual void pushEdge(DependencyGraph::Edge *edge) override {
        W.push(edge);
        if (edge->is_negated) {
            N.push(edge);
        }
    }
    virtual void pushDependency(DependencyGraph::Edge *edge) override {
        D.push(edge);
    }

    virtual void pushMessage(Message &message) override {
        M.push(message);
    }

    virtual void releaseNegationEdges(int dist) override {
        N.releaseNegationEdges(dist);
    }

    virtual TaskType pickTask(DependencyGraph::Edge*& edge, Message& message) override {
        if (D.pop(edge)) {
            return EDGE;
        } else if (N.pop(edge)) {
            return EDGE;
        } else if (M.pop(message)) {
            return MESSAGE;
        } else if (W.pop(edge)) {
            return EDGE;
        } else if (!N.empty()) {
            return UNAVAILABLE;
        } else {
            return EMPTY;
        }
    }

    virtual TaskType pickTask(DependencyGraph::Edge*& edge) override {
        SearchStrategy::Message m;
        TaskType t = pickTask(edge, m);
        assert(t != MESSAGE);
        while(t == UNAVAILABLE) {
            N.releaseNegationEdges(N.maxDistance());
            t = pickTask(edge, m);
        }
        return t;
    }

protected:
    EdgeList W;
    MessageList M;
    NegationList N;
    DependencyList D;
};

}
#endif // BASICSEARCHSTRATEGY_H
