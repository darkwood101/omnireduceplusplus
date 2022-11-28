#include <stdexcept>
#include <cassert>
#include <iostream>

#include "event.h"
#include "worker.h"
#include "aggregator.h"
#include "utils.h"

Worker::Worker(workernum_t id, uint32_t block_size) :
    id_(id),
    generator_(std::random_device{}()),
    block_size_(block_size),
    next_nonzero_(0),
    next_agg_(0),
    recv_block_(block_size_),
    send_block_(block_size_) {
}

void Worker::generate_data(size_t size, float sparsity) {
    if (sparsity < 0.0 || sparsity > 1.0) {
        throw std::invalid_argument("Sparsity must be between 0 and 1");
    }

    std::uniform_real_distribution<> distr(0.0, 1.0);
    gradients_.resize(size);
    for (size_t i = 0; i != size; ++i) {
        float num = distr(generator_);
        if (num <= sparsity) {
            gradients_[i] = 0;
        } else {
            gradients_[i] = distr(generator_);
        }
    }
}

void Worker::recv_block(const Block& block) {
    recv_block_ = block;
}

timedelta_t Worker::process_response() {
    DEBUG(std::cout
          << "[W" << id_
          << "] Processing block " << recv_block_.block_id_
          << " from aggregator, next requested is " << recv_block_.next_
          << std::endl;);
    for (size_t i = 0; i != recv_block_.data_.size(); ++i) {
        gradients_[recv_block_.block_id_ * block_size_ + i] = recv_block_.data_[i];
    }
    next_agg_ = recv_block_.next_;

    // How much time will it take to prepare for sending
    if (next_nonzero_ == BLOCK_INF || next_agg_ != next_nonzero_) {
        return 0;
    }
    timedelta_t copy_gradients = ceil(0.00064971 * block_size_);
    blocknum_t next_nonzero = find_nonzero();
    timedelta_t lookahead;
    if (next_nonzero == BLOCK_INF) {
        lookahead = ceil(0.00064871 * block_size_ * (gradients_.size() / block_size_ - next_agg_));
    } else {
        lookahead = ceil(0.00064871 * block_size_ * (next_nonzero - next_agg_));
    }
    return copy_gradients + lookahead;
}

timedelta_t Worker::prepare_to_send() {
    DEBUG(std::cout
          << "[W" << id_
          << "] Prepare to send, next requested block is " << next_agg_
          << ", next available is " << next_nonzero_
          << std::endl;);
    if (next_nonzero_ == BLOCK_INF || next_agg_ != next_nonzero_) {
        return 0;
    }

    for (size_t i = 0; i != block_size_; ++i) {
        send_block_.data_[i] = gradients_[next_agg_ * block_size_ + i];
    }
    send_block_.worker_id_ = id_;
    send_block_.block_id_ = next_agg_;

    // Find the next non-zero block
    next_nonzero_ = find_nonzero();
    send_block_.next_ = next_nonzero_;

    // Time taken to send over the network
    return ceil(90 + 0.8 * block_size_);
}

timedelta_t Worker::send(Aggregator& agg) {
    DEBUG(std::cout
          << "[W" << id_
          << "] Sending block " << send_block_.block_id_
          << " to aggregator"
          << std::endl;);
    agg.recv_block(send_block_);
    return ceil(0.00064971 * block_size_);
}

blocknum_t Worker::find_nonzero() const {
    blocknum_t next_nonzero = BLOCK_INF;
    for (blocknum_t i = next_agg_ + 1; i * block_size_ < gradients_.size(); ++i) {
        bool zero_block = true;
        for (size_t j = 0; j != block_size_; ++j) {
            if (gradients_[i * block_size_ + j] != 0) {
                zero_block = false;
                break;
            }
        }
        if (!zero_block) {
            next_nonzero = i;
            break;
        }
    }
    return next_nonzero;
}
