#include <stdexcept>
#include <cassert>

#include "event.h"
#include "worker.h"
#include "aggregator.h"

Worker::Worker(workernum_t id) :
    id_(id),
    generator_(rd_()) {
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

TimeDelta Worker::process_response() {
    for (size_t i = 0; i != recv_block_.data_.size(); ++i) {
        gradients_[recv_block_.block_id_ * block_size_ + i] = recv_block_.data_[i];
    }
    next_agg_ = recv_block_.next_;

    return recv_block_.data_.size();
}

TimeDelta Worker::prepare_to_send() {
    if (next_agg_ != next_nonzero_) {
        return 0;
    }

    for (size_t i = 0; i != block_size_; ++i) {
        send_block_.data_[i] = gradients_[next_agg_ * block_size_ + i];
    }
    send_block_.worker_id_ = id_;
    send_block_.block_id_ = next_agg_;

    // Find the next non-zero block
    blocknum_t old_nonzero = next_nonzero_;
    next_nonzero_ = BLOCK_INF;
    for (blocknum_t i = next_agg_ + 1; i * block_size_ < gradients_.size(); ++i) {
        bool zero_block = true;
        for (size_t j = 0; j != block_size_; ++j) {
            if (gradients_[i * block_size_ + j] != 0) {
                zero_block = false;
                break;
            }
        }
        if (!zero_block) {
            next_nonzero_ = i;
            break;
        }
    }
    send_block_.next_ = next_nonzero_;

    return block_size_ + (next_nonzero_ - old_nonzero) * block_size_;
}

TimeDelta Worker::send(Aggregator& agg) {
    if (next_agg_ != next_nonzero_) {
        return 0;
    }

    agg.recv_block(send_block_);
    return send_block_.data_.size();
}
