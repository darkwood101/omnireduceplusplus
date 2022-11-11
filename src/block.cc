#include "block.h"

Block::Block(uint32_t block_size) :
    next_(0),
    block_id_(0),
    worker_id_(0) {
    data_.resize(block_size);
}
