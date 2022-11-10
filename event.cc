#include "event.h"

#include <queue>
#include <iostream>

Event::Event(Timestamp start_timestamp, Timestamp end_timestamp) :
    start_timestamp_(start_timestamp),
    end_timestamp_(end_timestamp) {
}

bool Event::operator<(const Event &rhs) const {
    return end_timestamp_ < rhs.end_timestamp_;
}

bool Event::operator>(const Event& rhs) const {
    return end_timestamp_ > rhs.end_timestamp_;
}
