#ifndef _WORKER_H_
#define _WORKER_H_

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <random>

#include "types.h"
#include "event.h"
#include "block.h"

class Aggregator;

class Worker {
public:
    const workernum_t id_;

    Worker(workernum_t id);
    void generate_data(size_t size, float sparsity);
    void recv_block(const Block& block);
    timedelta_t process_response();
    timedelta_t prepare_to_send();
    timedelta_t send(Aggregator& agg);

private:
    std::mt19937 generator_;

    std::vector<float> gradients_;
    uint32_t block_size_ = 64;

    blocknum_t next_nonzero_;
    blocknum_t next_agg_;

    Block recv_block_;
    Block send_block_;
};

#endif
