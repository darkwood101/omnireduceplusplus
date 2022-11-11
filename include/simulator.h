#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include <queue>

#include "event.h"
#include "aggregator.h"
#include "worker.h"

class Simulator {
    using EventQueue = std::priority_queue<Event, std::vector<Event>, std::greater<Event>>;

public:
    Simulator(workernum_t num_workers);
    void generate_data(size_t size, float sparsity);
    void run();

private:
    Aggregator aggregator_;
    std::vector<Worker> workers_;

    uint32_t block_size_;
    uint64_t time_;         // global time
    EventQueue events_;
};

#endif
