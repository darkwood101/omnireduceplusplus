#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <queue>
#include "event.h"

class Context {
    using EventQueue = std::priority_queue<Event, std::vector<Event>, std::greater<Event>>;

public:

private:
    uint64_t time_;
    EventQueue events_;
};

#endif
