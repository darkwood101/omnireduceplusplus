#ifndef _EVENT_H_
#define _EVENT_H_

#include <cstdint>

using TimeDelta = uint64_t;
using Timestamp = uint64_t;

class Event {
public:
    Event(Timestamp start_timestamp, Timestamp end_timestamp);
    bool operator<(const Event& rhs) const;
    bool operator>(const Event& rhs) const;

private:
    Timestamp start_timestamp_;
    Timestamp end_timestamp_;
};

#endif
