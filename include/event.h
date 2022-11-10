#ifndef _EVENT_H_
#define _EVENT_H_

#include <cstdint>

#include "types.h"

enum EventType {
    WORKER_PROCESS,
    WORKER_PREPARE,
    WORKER_SEND,
    AGGREGATOR_PROCESS,
    AGGREGATOR_PREPARE,
    AGGREGATOR_SEND
};

struct Event {
    Event(EventType type,
          workernum_t worker_id,
          timestamp_t start_timestamp,
          timestamp_t end_timestamp);
    bool operator<(const Event& rhs) const;
    bool operator>(const Event& rhs) const;

    EventType type_;
    workernum_t worker_id_;
    timestamp_t start_timestamp_;
    timestamp_t end_timestamp_;
};

#endif
