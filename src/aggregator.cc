#include <stdexcept>
#include <cassert>
#include <iostream>

#include "aggregator.h"
#include "worker.h"
#include "utils.h"

Aggregator::Aggregator(workernum_t num_workers, uint32_t block_size, uint32_t bf_width) :
    num_workers_(num_workers),
    num_received_(0),
    num_to_receive_(num_workers_),
    num_sent_(0),
    block_size_(block_size),
    bf_width_(bf_width),
    send_packet_(block_size_, bf_width_) {
    for (size_t i = 0; i != num_workers_; ++i) {
        recv_packets_.push_back(Packet(block_size_, bf_width_));
    }
    min_next_.resize(bf_width_);
    std::fill(min_next_.begin(), min_next_.end(), BLOCK_INF);
}

void Aggregator::recv_packet(const Packet& packet) {
    // Sanity check -- cannot receive block from
    // non-existent worker
    debug_assert(packet.worker_id_ < num_workers_);
    recv_packets_[packet.worker_id_] = packet;
}

timedelta_t Aggregator::process_response(workernum_t worker) {
    // Sanity check -- cannot receive block from
    // non-existent worker
    debug_assert(worker < num_workers_);

    const Packet& recv_packet = recv_packets_[worker];
    verbose_print("[A]  Processing packet from worker " << worker
        << std::endl;);

    for (uint32_t i = 0; i != bf_width_; ++i) {
        const Block& recv_block = recv_packet.blocks_[i];
        verbose_print("     Processing block ID "
            << (recv_block.is_valid() ? std::to_string(recv_block.block_id_) : "INF")
            << ", next block ID "
            << (recv_block.is_next_valid() ? std::to_string(recv_block.next_) : "INF")
            << std::endl);

        Block& send_block = send_packet_.blocks_[i];
        // If the block is invalid, skip it
        if (!recv_block.is_valid()) { 
            continue;
        }
        // Sanity check -- the block ID must correspond to this column in the packet
        debug_assert(recv_block.block_id_ % bf_width_ == i);

        // Aggregate the gradients from the block
        for (size_t j = 0; j != recv_block.data_.size(); ++j) {
            send_block.data_[j] += recv_block.data_[j];
        }

        // Initially, send_block is invalid. The first received block will set
        // the ID, and all subsequently received blocks must have the same ID
        if (!send_block.is_valid()) {
            send_block.block_id_ = recv_block.block_id_;
        } else {
            debug_assert(send_block.block_id_ == recv_block.block_id_);
        }

        // Update the next block to be expected for this column
        min_next_[i] = std::min(min_next_[i], recv_block.next_);
    }

    ++num_received_;

    // Preparing to send will require iterating over all blocks in all
    // packets that the workers send
    return ceil(0.00064971 * bf_width_ * num_to_receive_);
}

timedelta_t Aggregator::prepare_to_send() {
    // Sanity check -- cannot prepare to send before receiving all
    // worker blocks
    debug_assert(num_received_ == num_to_receive_);

    // We need to count how many workers will send packets.
    // Workers where all next blocks in the packet are larger than min_next
    // will send nothing. So we go and count how many workers have
    // at least 1 valid block. But we must be careful not to double-count
    // for the same worker, so we use a vector of chars to label whether
    // a worker has already been counted or not.
    num_to_receive_ = 0;
    std::vector<char> recv;
    recv.resize(num_workers_);
    std::fill(recv.begin(), recv.end(), 0);
    for (uint32_t i = 0; i != bf_width_; ++i) {
        blocknum_t next_larger = BLOCK_INF;
        for (uint32_t j = 0; j != num_workers_; ++j) {
            debug_assert(recv_packets_[j].blocks_[i].next_ >= min_next_[i]);
            // If the next block is exactly the same as min_next, this worker will be sending
            // the packet.
            if (recv_packets_[j].blocks_[i].next_ == min_next_[i] && min_next_[i] != BLOCK_INF) {
                num_to_receive_ += (recv[j] == 0);
                recv[j] = 1;
                // Invalidate next for the next round
                recv_packets_[j].blocks_[i].next_ = BLOCK_INF;
            } else {
                // Maintain the next minimum block to ask for
                next_larger = std::min(next_larger, recv_packets_[j].blocks_[i].next_);
            }
        }
        send_packet_.blocks_[i].next_ = min_next_[i];
        min_next_[i] = next_larger;
    }
    send_packet_.worker_id_ = WORKER_ALL;

    // Verbose output and debug asserts, this loop is optimized out otherwise
    verbose_print("[A]  Prepared to send packet to all workers" << std::endl);
    for (uint32_t i = 0; i != bf_width_; ++i) {
        Block& send_block = send_packet_.blocks_[i];
        verbose_print("     Block ID "
            << (send_block.is_valid() ? std::to_string(send_block.block_id_) : "INF")
            << ", requesting next block ID "
            << (send_block.is_next_valid() ? std::to_string(send_block.next_) : "INF")
            << std::endl);
        if (!send_block.is_valid()) {
            debug_assert(!send_block.is_next_valid());
        }
    }

    // Count how many valid blocks will be sent
    uint32_t valid_blocks = 0;
    for (uint32_t i = 0; i != bf_width_; ++i) {
        valid_blocks += send_packet_.blocks_[i].is_valid();
    }
    // We should have at least one valid block. If there were no
    // valid blocks, then the simulation would have ended with workers
    // preparing to send.
    debug_assert(valid_blocks > 0);
    // The aggregator sends only the valid blocks
    return ceil(1 + 0.00008 * block_size_ * valid_blocks);
}

timedelta_t Aggregator::send(Worker& worker) {
    verbose_print("[A]  Sent packet to worker " << worker.id_ << std::endl);
    worker.recv_packet(send_packet_);
    ++num_sent_;
    uint32_t valid_blocks = 0;
    for (uint32_t i = 0; i != bf_width_; ++i) {
        valid_blocks += send_packet_.blocks_[i].is_valid();
    }
    // Processing the packet will take iterating over each fused block,
    // and then over data for valid blocks
    return ceil(0.00064971 * bf_width_ + 0.00064971 * valid_blocks * block_size_);
}

bool Aggregator::all_received() const {
    return num_received_ == num_to_receive_;
}

bool Aggregator::all_sent() const {
    return num_sent_ == num_workers_;
}

void Aggregator::reset() {
    num_received_ = 0;
    num_sent_ = 0;
    // We must invalidate blocks in the sending slot to make sure
    // that blocks that are skipped in prepare_to_send in the next round
    // will not be sent again.
    for (uint32_t i = 0; i != bf_width_; ++i) {
        send_packet_.blocks_[i].invalidate();
        std::fill(send_packet_.blocks_[i].data_.begin(),
                  send_packet_.blocks_[i].data_.end(),
                  0.0);
    }
}
