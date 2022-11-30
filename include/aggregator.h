#ifndef _AGGREGATOR_H_
#define _AGGREGATOR_H_

//#include <cstdlib>
#include <cstdint>
#include <queue>

#include "types.h"
#include "event.h"
#include "block.h"

class Worker;

class Aggregator {
public:
    Aggregator(workernum_t num_workers, uint32_t block_size, uint32_t bf_width);

    // Copy the packet from a worker to the aggregator
    void recv_packet(const Packet& packet);

    // Process the response from a given worker,
    // returns the time needed for the *next* step (prepare to send)
    timedelta_t process_response(workernum_t worker);

    // Prepare to send the packet,
    // returns the time needed for the *next* step (send)
    timedelta_t prepare_to_send();

    // Send the packet to a given worker,
    // returns the time needed for the *next* step (worker process)
    timedelta_t send(Worker& worker);

    // Returns true iff packets from all required workers
    // have been received in this round
    bool all_received() const;

    // Returns true iff packets have been sent to all workers
    // in this round
    bool all_sent() const;

    // Resets per-round state
    void reset();

private:
    const workernum_t num_workers_;

    // How many packets received so far in this round
    uint32_t num_received_;

    // How many packets expected to receive in this round
    // (i.e. how many workers are required)
    uint32_t num_to_receive_;

    // How many packets sent so far in this round
    uint32_t num_sent_;

    // Aggregation block size, set at construction time
    const uint32_t block_size_;

    // Block fusion width, set at construction time
    const uint32_t bf_width_;

    // Minimum next nonzero blocks for each block in the fused packet
    // (i.e. the blocks to ask for in the next round)
    // min_next_.size() == bf_width_
    std::vector<blocknum_t> min_next_;

    // Packets received from workers
    // recv_blocks_.size() == num_workers_
    std::vector<Packet> recv_packets_;

    // Packet to be multicast to workers
    Packet send_packet_;
};

#endif
