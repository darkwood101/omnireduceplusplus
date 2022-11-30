#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include <queue>

#include "event.h"
#include "aggregator.h"
#include "worker.h"

class Simulator {
    using EventQueue = std::priority_queue<Event, std::vector<Event>, std::greater<Event>>;

public:
    Simulator(workernum_t num_workers, uint32_t block_size, uint32_t bf_width);
    void generate_data(size_t size, float sparsity);
    void run();
    uint64_t get_time();

#ifndef DEBUGGING
    private:
#else
    public:
#endif
    Aggregator aggregator_;
    std::vector<Worker> workers_;

    // Block size, set at construction time
    const uint32_t block_size_;

    // Block fusion width, set at construction time
    const uint32_t bf_width_;

    // Global time
    uint64_t time_;

    EventQueue events_;
};

#endif
