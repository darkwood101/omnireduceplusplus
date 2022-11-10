#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <vector>
#include <cstdint>
#include <cstdlib>

#include "types.h"

struct Block {
    std::vector<float> data_;
    blocknum_t next_;
    blocknum_t block_id_;
    workernum_t worker_id_;
};

#endif
