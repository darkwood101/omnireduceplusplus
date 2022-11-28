#include <stdexcept>
#include <cassert>
#include <iostream>

#include "aggregator.h"
#include "worker.h"
#include "utils.h"

Aggregator::Aggregator(workernum_t num_workers) :
    num_workers_(num_workers),
    num_received_(0),
    num_to_receive_(num_workers_),
    send_block_(block_size_) {
    for (size_t i = 0; i != num_workers_; ++i) {
        recv_blocks_.push_back(Block(block_size_));
    }
    min_next_ = BLOCK_INF;
}

void Aggregator::recv_block(const Block &block) {
    // Sanity check -- cannot receive block from
    // non-existent worker
    debug_assert(block.worker_id_ < num_workers_);
    recv_blocks_[block.worker_id_] = block;
}

timedelta_t Aggregator::process_response(workernum_t worker) {
    Block& recv_block = recv_blocks_[worker];
    DEBUG(std::cout
          << "[A]  Processing block " << recv_block.block_id_
          << " from worker " << worker
          << ", next " << recv_block.next_
          << std::endl;);

    // Copy block data
    for (size_t i = 0; i != recv_block.data_.size(); ++i) {
        send_block_.data_[i] += recv_block.data_[i];
    }
    ++num_received_;

    // For all arrived blocks other than the first one,
    // this will overwrite the ID with the same value
    send_block_.block_id_ = recv_block.block_id_;

    // Update the next block to be expected
    min_next_ = std::min(min_next_, recv_block.next_);

    return ceil(0.00064971 * recv_blocks_.size());
}

timedelta_t Aggregator::prepare_to_send() {
    // Sanity check -- cannot prepare to send before receiving all
    // worker blocks
    debug_assert(num_received_ == num_to_receive_);

    // Prepare for responses
    num_to_receive_ = 0;
    blocknum_t next_larger = BLOCK_INF;
    for (size_t i = 0; i != recv_blocks_.size(); ++i) {
        debug_assert(recv_blocks_[i].next_ >= min_next_);
        // Count how many blocks to receive in the next round; this the number
        // of workers whose next non-zero block is the same as the overall
        // minimum next non-zero block
        if (recv_blocks_[i].next_ == min_next_ && min_next_ != BLOCK_INF) {
            ++num_to_receive_;
            recv_blocks_[i].next_ = BLOCK_INF;
        }
        // Maintain the next minimum block to ask for
        else {
            next_larger = std::min(next_larger, recv_blocks_[i].next_);
        }
    }
    send_block_.worker_id_ = WORKER_ALL;
    send_block_.next_ = min_next_;

    min_next_ = next_larger;
    num_received_ = 0;

    DEBUG(std::cout
          << "[A]  Sending block " << send_block_.block_id_
          << " to all workers, requesting block " << send_block_.next_
          << std::endl;);

    return ceil(90 + 0.8 * block_size_ * num_workers_);
}

timedelta_t Aggregator::send(Worker& worker) {
    DEBUG(std::cout
          << "[A]  Sending block " << send_block_.block_id_
          << " to workers"
          << std::endl;);
    worker.recv_block(send_block_);
    return ceil(0.00064971 * block_size_);
}

bool Aggregator::round_done() const {
    return num_received_ == num_to_receive_;
}

bool Aggregator::all_done() const {
    return num_to_receive_ == 0;
}
