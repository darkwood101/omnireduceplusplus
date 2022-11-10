#ifndef _AGGREGATOR_H_
#define _AGGREGATOR_H_

//#include <cstdlib>
#include <cstdint>
#include <queue>

#include "types.h"
#include "event.h"
#include "block.h"

class Worker;

class Aggregator {
public:
    Aggregator(workernum_t num_workers);
    void recv_block(const Block& block);
    timedelta_t process_response(workernum_t worker);
    timedelta_t prepare_to_send();
    timedelta_t send(Worker& worker);
    bool round_done() const;
    bool all_done() const;

private:
    const workernum_t num_workers_;
    uint32_t num_received_;
    uint32_t num_to_receive_;
    blocknum_t min_next_;
    std::vector<Block> recv_blocks_;
    Block send_block_;
};

#endif
