#include <cassert>

#include "simulator.h"
#include "worker.h"

Simulator::Simulator(workernum_t num_workers) :
    aggregator_(num_workers) {
    for (workernum_t worker_id = 0; worker_id != num_workers; ++worker_id) {
        Worker w = {worker_id};
        workers_.push_back(w);
    }
}

void Simulator::generate_data(size_t size, float sparsity) {
    for (Worker& w : workers_) {
        w.generate_data(size, sparsity);
    }
}

void Simulator::run() {
    while (!events_.empty()) {
        const Event& e = events_.top();
        assert(e.start_timestamp_ <= time_);
        assert(e.worker_id_ < workers_.size());
        Worker& worker = workers_[e.worker_id_];
        assert(worker.id_ == e.worker_id_);
        timedelta_t delta;
        switch (e.type_) {
            case WORKER_PROCESS: {
                delta = worker.prepare_to_send();
                if (delta != TIME_NOW) {
                    events_.push(Event(WORKER_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            }
            case WORKER_PREPARE: {
                delta = worker.send(aggregator_);
                assert(delta != TIME_NOW);
                events_.push(Event(WORKER_SEND, worker.id_, time_, time_ + delta));
                break;
            }
            case WORKER_SEND: {
                delta = aggregator_.process_response(worker.id_);
                events_.push(Event(AGGREGATOR_PROCESS, worker.id_, time_, time_ + delta));
                break;
            }
            case AGGREGATOR_PROCESS: {
                if (aggregator_.round_done()) {
                    delta = aggregator_.prepare_to_send();
                    events_.push(Event(AGGREGATOR_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            }
            case AGGREGATOR_PREPARE: {
                for (Worker& w : workers_) {
                    delta = aggregator_.send(worker);
                    events_.push(Event(AGGREGATOR_SEND, w.id_, time_, time_ + delta));
                }
                break;
            }
            case AGGREGATOR_SEND: {
                delta = worker.process_response();
                events_.push(Event(WORKER_PROCESS, worker.id_, time_, time_ + delta));
            }
        }
    }
    assert(aggregator_.all_done());
}
