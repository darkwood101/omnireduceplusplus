#include <cassert>
#include <iostream>

#include "simulator.h"
#include "worker.h"

Simulator::Simulator(workernum_t num_workers) :
    aggregator_(num_workers),
    time_(0) {
    for (workernum_t worker_id = 0; worker_id != num_workers; ++worker_id) {
        Worker w = {worker_id};
        workers_.push_back(w);
    }
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
        assert(e.start_timestamp_ <= time_);
        assert(e.worker_id_ < workers_.size());
        Worker& worker = workers_[e.worker_id_];
        assert(worker.id_ == e.worker_id_);
        timedelta_t delta;
        time_ = e.end_timestamp_;
        switch (e.type_) {
            case INIT_EVENT: {
                for (Worker& w : workers_) {
                    events_.push(Event(WORKER_PREPARE, w.id_, time_, time_));
                }
                break;
            }
            case WORKER_PROCESS: {
                delta = worker.process_response();
                if (delta != TIME_NOW) {
                    events_.push(Event(WORKER_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            }
            case WORKER_PREPARE: {
                delta = worker.prepare_to_send();
                if (delta != TIME_NOW) {
                    events_.push(Event(WORKER_SEND, worker.id_, time_, time_ + delta));
                }
                /*
                delta = worker.send(aggregator_);
                assert(delta != TIME_NOW);
                events_.push(Event(WORKER_SEND, worker.id_, time_, time_ + delta));*/
                break;
            }
            case WORKER_SEND: {
                delta = worker.send(aggregator_);
                events_.push(Event(AGGREGATOR_PROCESS, worker.id_, time_, time_ + delta));
                break;
            }
            case AGGREGATOR_PROCESS: {
                delta = aggregator_.process_response(worker.id_);
                if (aggregator_.round_done()) {
                    events_.push(Event(AGGREGATOR_PREPARE, worker.id_, time_, time_ + delta));
                }
                break;
            }
            case AGGREGATOR_PREPARE: {
                delta = aggregator_.prepare_to_send();
                for (Worker& w : workers_) {
                    events_.push(Event(AGGREGATOR_SEND, w.id_, time_, time_ + delta));
                }
                break;
            }
            case AGGREGATOR_SEND: {
                delta = aggregator_.send(worker);
                events_.push(Event(WORKER_PROCESS, worker.id_, time_, time_ + delta));
                break;
            }
        }
        events_.pop();
    }
    assert(aggregator_.all_done());
}
