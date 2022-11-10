#include "event.h"

#include <queue>
#include <iostream>

Event::Event(EventType type,
             workernum_t worker_id,
             timestamp_t start_timestamp,
             timestamp_t end_timestamp) :
    type_(type),
    worker_id_(worker_id),
    start_timestamp_(start_timestamp),
    end_timestamp_(end_timestamp) {
}

bool Event::operator<(const Event &rhs) const {
    return end_timestamp_ < rhs.end_timestamp_;
}

bool Event::operator>(const Event& rhs) const {
    return end_timestamp_ > rhs.end_timestamp_;
}
