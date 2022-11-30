#include <cassert>
#include <iostream>

#include "simulator.h"
#include "worker.h"
#include "utils.h"

Simulator::Simulator(workernum_t num_workers, uint32_t block_size, uint32_t bf_width) :
    aggregator_(num_workers, block_size, bf_width),
    block_size_(block_size),
    bf_width_(bf_width),
    time_(0) {
    // Initialize all workers
    for (workernum_t worker_id = 0; worker_id != num_workers; ++worker_id) {
        Worker w = {worker_id, block_size, bf_width};
        workers_.push_back(w);
    }
    // Fake event to kickstart the simulator
    events_.push(Event(INIT_EVENT, 0, 0, 0));
}

void Simulator::generate_data(size_t size, float sparsity) {
    // For now, to keep things a bit simpler, we require that data size
    // be a multiple of block size
    if (size % block_size_ != 0) {
        throw std::invalid_argument("Data size must be multiple of block size");
    }
    for (Worker& w : workers_) {
        w.generate_data(size, sparsity);
    }
}

void Simulator::run() {
    while (!events_.empty()) {
        const Event& e = events_.top();
        Worker& worker = workers_[e.worker_id_];

        // Sanity checks
        debug_assert(e.start_timestamp_ <= time_);
        debug_assert(e.worker_id_ < workers_.size());
        debug_assert(e.end_timestamp_ >= time_);
        debug_assert(worker.id_ == e.worker_id_);

        // Advance time
        time_ = e.end_timestamp_;
        timedelta_t delta;
        verbose_print("[TIMESTAMP: " << time_ << "]" << std::endl);
        switch (e.type_) {
            case INIT_EVENT:
                // Workers will first prepare to send
                for (Worker& w : workers_) {
                    events_.push(Event(WORKER_PREPARE, w.id_, time_, time_));
                }
                break;
            case WORKER_PROCESS:
                // Once the worker processed the packet, prepare for sending
                delta = worker.process_response();
                events_.push(Event(WORKER_PREPARE, worker.id_, time_, time_ + delta));
                break;
            case WORKER_PREPARE:
                delta = worker.prepare_to_send();
                // If preparation is immediate, requested packet is of lower number than the
                // worker's next nonzero block, so don't send anything
                if (delta != TIME_NOW) {
                    events_.push(Event(WORKER_SEND, worker.id_, time_, time_ + delta));
                }
                break;
            case WORKER_SEND:
                // Once the worker sends the packet, aggregator should process it
                delta = worker.send(aggregator_);
                events_.push(Event(AGGREGATOR_PROCESS, worker.id_, time_, time_ + delta));
                break;
            case AGGREGATOR_PROCESS:
                delta = aggregator_.process_response(worker.id_);
                // Once the aggregator processes the packet, it should prepare to send,
                // but only if all required workers sent their packets
                if (aggregator_.all_received()) {
                    events_.push(Event(AGGREGATOR_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            case AGGREGATOR_PREPARE:
                // Once the aggregator prepared to send, it multicasts the packet to all workers
                delta = aggregator_.prepare_to_send();
                for (Worker& w : workers_) {
                    events_.push(Event(AGGREGATOR_SEND, w.id_, time_, time_ + delta));
                }
                break;
            case AGGREGATOR_SEND:
                // Once a worker receives the block, it processes it
                delta = aggregator_.send(worker);
                // Reset per-round aggregator state if packets have been sent to all workers
                if (aggregator_.all_sent()) {
                    aggregator_.reset();
                }
                events_.push(Event(WORKER_PROCESS, worker.id_, time_, time_ + delta));
                break;
        }
        events_.pop();
    }
}

uint64_t Simulator::get_time() {
    return time_;
}
