#include "block.h"

Block::Block(uint32_t block_size) :
    next_(BLOCK_INF),
    block_id_(BLOCK_INF) {
    data_.resize(block_size);
}

bool Block::is_valid() const {
    return block_id_ != BLOCK_INF;
}

bool Block::is_next_valid() const {
    return next_ != BLOCK_INF;
}

void Block::invalidate() {
    block_id_ = BLOCK_INF;
}

Packet::Packet(uint32_t block_size, uint32_t bf_width) {
    for (uint32_t i = 0; i != bf_width; ++i) {
        blocks_.push_back(Block(block_size));
    }
}
