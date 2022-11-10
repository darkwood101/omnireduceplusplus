#ifndef _WORKER_H_
#define _WORKER_H_

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <random>

#include "event.h"
#include "block.h"

class Aggregator;

class Worker {
public:
    Worker(uint32_t id);
    void generate_data(size_t size, float sparsity);
    void recv_block(const Block& block);
    TimeDelta process_response();
    TimeDelta prepare_to_send();
    TimeDelta send(Aggregator& agg);

private:
    std::random_device rd_;
    std::mt19937 generator_;

    std::vector<float> gradients_;
    uint32_t block_size_;
    uint32_t id_;

    size_t next_nonzero_;
    size_t next_agg_;

    Block recv_block_;
    Block send_block_;
};

#endif
