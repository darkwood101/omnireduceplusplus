#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <vector>
#include <cstdint>
#include <cstdlib>

#include "types.h"

struct Block {
    // Initializes the block as an invalid block
    Block(uint32_t block_size);

    // Gradients in the block
    // data_.size() == block_size * sizeof(float)
    std::vector<float> data_;

    // Next non-zero block ID in this fusion column
    blocknum_t next_;

    // This block ID
    blocknum_t block_id_;

    bool is_valid() const;
    bool is_next_valid() const;
    void invalidate();
};

struct Packet {
    // Initializes a packet of invalid blocks
    Packet(uint32_t block_size, uint32_t bf_width);

    // The array of blocks in a packet
    // blocks_.size() == bf_width
    std::vector<Block> blocks_;

    // The worker id where this packet originated
    // Unused when the packet is sent from the aggregator to the workers
    workernum_t worker_id_;
};

#endif
