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
    Aggregator(workernum_t num_workers);

    // Copy the block from a worker to the aggregator
    void recv_block(const Block& block);

    // Process the response from a given worker
    timedelta_t process_response(workernum_t worker);

    // Prepare to send the block
    timedelta_t prepare_to_send();

    // Send the block to a given worker
    timedelta_t send(Worker& worker);

    // Returns true iff blocks from all required workers
    // have been received
    bool round_done() const;

    // Requires true iff the communication is done
    bool all_done() const;

private:
    const workernum_t num_workers_;

    // How many blocks received so far in this round
    uint32_t num_received_;
    
    // How many blocks expected to receive in this round
    // (i.e. how many workers are required)
    uint32_t num_to_receive_;

    // TODO Block size, statically fixed for now
    uint32_t block_size_ = 64;

    // Minimum next nonzero block
    // (i.e. the block to ask for in the next round)
    blocknum_t min_next_;

     // Blocks received from workers
    std::vector<Block> recv_blocks_;

    // Block to be multicast to workers
    Block send_block_;
};

#endif
