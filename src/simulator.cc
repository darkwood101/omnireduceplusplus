#include <cassert>
#include <iostream>

#include "simulator.h"
#include "worker.h"
#include "utils.h"

Simulator::Simulator(workernum_t num_workers, uint32_t block_size) :
    aggregator_(num_workers, block_size),
    block_size_(block_size),
    time_(0) {
    for (workernum_t worker_id = 0; worker_id != num_workers; ++worker_id) {
        Worker w = {worker_id, block_size};
        workers_.push_back(w);
    }
    // Fake event to kickstart the simulator
    events_.push(Event(INIT_EVENT, 0, 0, 0));
}

void Simulator::generate_data(size_t size, float sparsity) {
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
        DEBUG(std::cout << "[" << time_ << "]";);
        switch (e.type_) {
            case INIT_EVENT:
                // Workers will first prepare to send
                for (Worker& w : workers_) {
                    events_.push(Event(WORKER_PREPARE, w.id_, time_, time_));
                }
                break;
            case WORKER_PROCESS:
                // Once the worker processed the block, prepare for sending
                delta = worker.process_response();
                events_.push(Event(WORKER_PREPARE, worker.id_, time_, time_ + delta));
                break;
            case WORKER_PREPARE:
                delta = worker.prepare_to_send();
                // If preparation is immediate, requested block is of lower number than the
                // worker's next nonzero block, so don't send anything.
                // Otherwise, send.
                if (delta != TIME_NOW) {
                    events_.push(Event(WORKER_SEND, worker.id_, time_, time_ + delta));
                }
                break;
            case WORKER_SEND:
                // Once the worker sends the block, aggregator should process it
                delta = worker.send(aggregator_);
                events_.push(Event(AGGREGATOR_PROCESS, worker.id_, time_, time_ + delta));
                break;
            case AGGREGATOR_PROCESS:
                delta = aggregator_.process_response(worker.id_);
                // Once the aggregator processed the block, it should prepare to send,
                // but only if all required workers sent their blocks
                if (aggregator_.round_done()) {
                    events_.push(Event(AGGREGATOR_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            case AGGREGATOR_PREPARE:
                // Once the worker prepares to send, it multicasts the block to all workers
                delta = aggregator_.prepare_to_send();
                for (Worker& w : workers_) {
                    events_.push(Event(AGGREGATOR_SEND, w.id_, time_, time_ + delta));
                }
                break;
            case AGGREGATOR_SEND:
                // Once a worker receives the block, it processes it
                delta = aggregator_.send(worker);
                events_.push(Event(WORKER_PROCESS, worker.id_, time_, time_ + delta));
                break;
        }
        events_.pop();
    }
    assert(aggregator_.all_done());
}
