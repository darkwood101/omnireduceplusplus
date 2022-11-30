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

    Worker(workernum_t id, uint32_t block_size, uint32_t bf_width);

    // Generate gradients with a given number of elements and a given sparsity
    void generate_data(size_t size, float sparsity);

    // Copy the packet from the aggregator to the worker
    void recv_packet(const Packet& packet);

    // Process the response from the aggregator
    timedelta_t process_response();

    // Prepare to send the packet to the aggregator
    timedelta_t prepare_to_send();

    // Send the packet to the aggregator
    timedelta_t send(Aggregator& agg);

#ifndef DEBUGGING
private:
#else
public:
#endif
    // Random number generator
    std::mt19937 generator_;

    // Worker gradients
    std::vector<float> gradients_;

    // Aggregation block size, set at construction time
    const uint32_t block_size_;

    // Block fusion width, set at construction time
    const uint32_t bf_width_;

    // The next nonzero blocks for each fused block in a packet
    // next_nonzero_.size() == bf_width_
    std::vector<blocknum_t> next_nonzero_;

    // The next block requested by the aggregator for each fused block in a packet
    // next_agg_.size() == bf_width_
    std::vector<blocknum_t> next_agg_;

    // Slot for receiving a packet from the aggregator
    Packet recv_packet_;

    // Slot for sending a packet to the aggregator
    Packet send_packet_;

    // Find the next non-zero block for each block in a fused packet,
    // to be called after process_response
    std::vector<blocknum_t> find_nonzero() const;
};

#endif
