#include "TypeMessageList.h"


bool SearchStrategy::TypeMessageList::pop(SearchStrategy::Message &m)
{
    if (!answers.empty()) {
        m = answers.front();
        answers.pop();
        return true;
    } else if (!halts.empty()) {
        m = halts.front();
        halts.pop();
        return true;
    } else if (!requests.empty()) {
        m = requests.front();
        requests.pop();
        return true;
    } else {
        return false;
    }
}

void SearchStrategy::TypeMessageList::push(SearchStrategy::Message &m)
{
    if (m.type == Message::HALT) {
        halts.push(m);
    } else if (m.type == Message::REQUEST) {
        requests.push(m);
    } else {
        answers.push(m);
    }
}

bool SearchStrategy::TypeMessageList::empty() const
{
    return answers.empty() && halts.empty() && requests.empty();
}
