#ifndef COLLECTION_H
#define COLLECTION_H

#include <stddef.h>
#include <queue>
#include <iostream>

namespace SearchStrategy {

//The WaitingList class defines an interface required by the universal search strategy.
//The class itself is essentially an adapter between whatever collection you want to
//use as your waiting list and the universal search strategy.
//Therefore you can either provide a specialization of this class for your list type
//or just make sure the list type matches this kind of interface.
//(If you can't implement this interface, you probably need to make your own strategy)

//default implementation matches the interface of a stack
template<class T, class Container>
class WaitingList {
public:
    Container c;
    WaitingList(Container S = Container()) : c(S){}
    //return true if pop was successful
    bool pop(T& e) {
        if (c.empty()) {
            return false;
        } else {
            e = c.top();
            c.pop();
            return true;
        }
    }
    void push(T& e) { c.push(e); }
    bool empty() const { return c.empty(); }
};

//a specialization matching the interface of a queue
template<class T>
class WaitingList<T, std::queue<T>> {
    using Container = std::queue<T>;
public:
    Container c;
    WaitingList(Container S = Container()) : c(S){}
    //return true if pop was successful
    bool pop(T& e) {
        if (c.empty()) {
            return false;
        } else {
            e = c.front();
            c.pop();
            return true;
        }
    }
    void push(T& e) { c.push(e); }
    bool empty() const { return c.empty(); }
};

}

#endif // COLLECTION_H
