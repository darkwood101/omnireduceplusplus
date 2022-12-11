#include <stdexcept>
#include <cassert>
#include <iostream>

#include "event.h"
#include "worker.h"
#include "aggregator.h"
#include "utils.h"

Worker::Worker(workernum_t id, uint32_t block_size, uint32_t bf_width) :
    id_(id),
    generator_(std::random_device{}()),
    block_size_(block_size),
    bf_width_(bf_width),
    recv_packet_(block_size, bf_width),
    send_packet_(block_size, bf_width) {
    // Initialize next blocks to first block in each column
    // (0, 1, 2, 3, ...)
    for (uint32_t i = 0; i != bf_width; ++i) {
        next_nonzero_.push_back(i);
        next_agg_.push_back(i);
    }
}

void Worker::generate_data(size_t size, float sparsity) {
    if (sparsity < 0.0 || sparsity > 1.0) {
        throw std::invalid_argument("Sparsity must be between 0 and 1");
    }

    std::uniform_real_distribution<> distr(0.0, 1.0);
    gradients_.resize(size);
    for (size_t i = 0; i != size; ++i) {
        float num = distr(generator_);
        // With probability sparsity, the number will be 0,
        // and randomly between 0 and 1 otherwise
        if (num <= sparsity) {
            gradients_[i] = 0;
        } else {
            gradients_[i] = distr(generator_);
        }
    }
}

void Worker::recv_packet(const Packet& packet) {
    // Sanity check -- the packet from the aggregator must be multicast
    debug_assert(packet.worker_id_ == WORKER_ALL);
    recv_packet_ = packet;
}

timedelta_t Worker::process_response() {
    verbose_print("[W" << id_
        << "] Processing packet from aggregator" << std::endl;);

    for (uint32_t i = 0; i != bf_width_; ++i) {
        Block& recv_block = recv_packet_.blocks_[i];
        verbose_print("     Processing block ID "
            << (recv_block.is_valid() ? std::to_string(recv_block.block_id_) : "INF")
            << ", next requested block ID "
            << (recv_block.is_next_valid() ? std::to_string(recv_block.next_) : "INF")
            << ", next available block ID "
            << (next_nonzero_[i] != BLOCK_INF ? std::to_string(next_nonzero_[i]) : "INF")
            << std::endl;
        );
        // Skip invalid blocks == blocks that were not sent by the aggregator
        if (!recv_block.is_valid()) {
            // If a block was not sent by the aggregator, then
            // the next requested block also must be invalid, and there
            // must be no valid blocks in this column anymore
            debug_assert(!recv_block.is_next_valid());
            debug_assert(next_nonzero_[i] == BLOCK_INF);
            continue;
        }
        debug_assert(recv_block.data_.size() == block_size_);

        // Copy gradients for each block in the fused packet
        for (size_t j = 0; j != recv_block.data_.size(); ++j) {
            gradients_[recv_block.block_id_ * block_size_ + j] = recv_block.data_[j];
        }   

        // Update the blocks requested by the aggregator
        next_agg_[i] = recv_block.next_;
    }

    timedelta_t total_time = 0;
    std::vector<blocknum_t> next_nonzero = find_nonzero();
    debug_assert(next_nonzero.size() == bf_width_);

    total_time += ceil(0.00064971 * bf_width_);
    for (uint32_t i = 0; i != bf_width_; ++i) {
        // Skip if there is no next non-zero block or if the block requested by the
        // aggregator is different
        if (next_nonzero_[i] == BLOCK_INF || next_agg_[i] != next_nonzero_[i]) {
            continue;
        }
        // Overhead of copying gradients
        total_time += ceil(0.00064971 * block_size_);
        // Lookahead overhead
        if (next_nonzero[i] == BLOCK_INF) {
            total_time += ceil(0.00064971 * block_size_ * (gradients_.size() / block_size_ - next_agg_[i]));
        } else {
            total_time += ceil(0.00064971 * block_size_ * (next_nonzero[i] - next_agg_[i]));
        }
    }
    return total_time;
}

timedelta_t Worker::prepare_to_send() {
    for (uint32_t i = 0; i != bf_width_; ++i) {
        Block& block = send_packet_.blocks_[i];
        // If there are no nonzero blocks left in the column, or the aggregator
        // requested a different, smaller block ID, then invalidate and skip this block
        if (next_nonzero_[i] == BLOCK_INF || next_agg_[i] != next_nonzero_[i]) {
            block.invalidate();
            continue;
        }
        block.block_id_ = next_agg_[i];
        // Sanity check -- the block ID must correspond to this column in the packet
        debug_assert(block.block_id_ % bf_width_ == i);
        // Copy the gradients
        for (size_t j = 0; j != block_size_; ++j) {
            block.data_[j] = gradients_[next_agg_[i] * block_size_ + j];
        }
    }
    send_packet_.worker_id_ = id_;

    // Find the next non-zero block for each block in the fused packet
    next_nonzero_ = find_nonzero();
    for (uint32_t i = 0; i != bf_width_; ++i) {
        send_packet_.blocks_[i].next_ = next_nonzero_[i];
    }

    verbose_print("[W" << id_
        << "] Prepared to send packet to aggregator"
        << std::endl);
    // Verbose output, this loop is optimized out otherwise
    for (uint32_t i = 0; i != bf_width_; ++i) {
        Block& send_block = send_packet_.blocks_[i];
        // Silence compiler warnings when verbose mode is turned off
        (void) send_block;
        verbose_print("     Block ID "
        << (send_block.is_valid() ? std::to_string(send_block.block_id_) : "INF")
        << ", next available "
        << (send_block.is_next_valid() ? std::to_string(send_block.next_) : "INF")
        << std::endl);
    }

    uint32_t valid_blocks = 0;
    for (uint32_t i = 0; i != bf_width_; ++i) {
        valid_blocks += send_packet_.blocks_[i].is_valid();
    }
    // If there are no valid blocks in the packet, the worker does not
    // send anything, so sending takes 0 time
    if (valid_blocks == 0) {
        return 0;
    }
    // Otherwise, the worker sends only the valid blocks
    return ceil(1 + 0.00008 * block_size_ * valid_blocks);
}

timedelta_t Worker::send(Aggregator& agg) {
    verbose_print("[W" << id_
          << "] Sent packet to aggregator"
          << std::endl);
    agg.recv_packet(send_packet_);
    uint32_t valid_blocks = 0;
    for (uint32_t i = 0; i != bf_width_; ++i) {
        valid_blocks += send_packet_.blocks_[i].is_valid();
    }
    // Processing the packet will take iterating over each fused block,
    // and then over data for valid blocks
    return ceil(0.00064971 * bf_width_ + 0.00064971 * valid_blocks * block_size_);
}

std::vector<blocknum_t> Worker::find_nonzero() const {
    std::vector<blocknum_t> next_nonzero;
    next_nonzero.resize(bf_width_);
    std::fill(next_nonzero.begin(), next_nonzero.end(), BLOCK_INF);

    for (uint32_t i = 0; i != bf_width_; ++i) {
        // For columns for which the aggregator requested INF,
        // there are no more nonzero blocks
        if (next_agg_[i] == BLOCK_INF) {
            continue;
        }
        // Iterate through blocks in the column until one with all zeros is found
        for (blocknum_t j = next_agg_[i] + bf_width_;
             j * block_size_ < gradients_.size();
             j += bf_width_) {
            bool zero_block = true;
            for (size_t k = 0;
                 k != block_size_ && j * block_size_ + k < gradients_.size();
                 ++k) {
                if (gradients_[j * block_size_ + k] != 0) {
                    zero_block = false;
                    break;
                }
            }
            if (!zero_block) {
                next_nonzero[i] = j;
                break;
            }
        }
    }
    return next_nonzero;
}
