#include <stdexcept>
#include <cassert>
#include <iostream>

#include "aggregator.h"
#include "worker.h"

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
    if (block.worker_id_ >= num_workers_) {
        throw std::invalid_argument("Invalid worker number");
    }
    recv_blocks_[block.worker_id_] = block;
}

timedelta_t Aggregator::process_response(workernum_t worker) {
    // Copy the data
    Block& recv_block = recv_blocks_[worker];
    std::cout << "[A]  Processing block " << recv_block.block_id_ << " from worker " << worker << ", next " << recv_block.next_ << std::endl;
    for (size_t i = 0; i != recv_block.data_.size(); ++i) {
        send_block_.data_[i] += recv_block.data_[i];
    }
    ++num_received_;

    send_block_.block_id_ = recv_block.block_id_;

    // Update the next block to be expected
    min_next_ = std::min(min_next_, recv_block.next_);

    return recv_blocks_[worker].data_.size();
}

timedelta_t Aggregator::prepare_to_send() {
    if (num_received_ != num_to_receive_) {
        throw std::logic_error("Can't prepare to send before receiving all worker packets");
    }

    // Prepare for responses
    num_to_receive_ = 0;
    blocknum_t next_larger = BLOCK_INF;
    for (size_t i = 0; i != recv_blocks_.size(); ++i) {
        assert(recv_blocks_[i].next_ >= min_next_);
        // Invalidate this `next_` number
        if (recv_blocks_[i].next_ == min_next_ && min_next_ != BLOCK_INF) {
            ++num_to_receive_;
            recv_blocks_[i].next_ = BLOCK_INF;
        } else {
            next_larger = std::min(next_larger, recv_blocks_[i].next_);
        }
    }
    send_block_.worker_id_ = WORKER_ALL;
    send_block_.next_ = min_next_;

    min_next_ = next_larger;
    num_received_ = 0;

    std::cout << "[A]  Sending block " << send_block_.block_id_ << " to all workers, requesting block " << send_block_.next_ << std::endl;

    return static_cast<timedelta_t>(num_workers_);
}

timedelta_t Aggregator::send(Worker& worker) {
    worker.recv_block(send_block_);
    return send_block_.data_.size();
}

bool Aggregator::round_done() const {
    return num_received_ == num_to_receive_;
}

bool Aggregator::all_done() const {
    return num_to_receive_ == 0;
}
