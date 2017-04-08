#ifndef TYPEMESSAGELIST_H
#define TYPEMESSAGELIST_H

#include <queue>
#include "iSearchStrategy.h"

//A waiting list for messages that has different queues for various message types
//effectively prioritising backproagation and halting.

namespace SearchStrategy {

class TypeMessageList
{
public:
    //return true if pop was successful
    bool pop(Message& m);
    void push(Message& m);
    bool empty() const;

protected:
    std::queue<Message> answers;
    std::queue<Message> halts;
    std::queue<Message> requests;
};


}

#endif // TYPEMESSAGELIST_H
