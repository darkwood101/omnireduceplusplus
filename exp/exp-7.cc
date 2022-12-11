#include <iostream>
#include <cassert>

#include "simulator.h"
#include "utils.h"

static constexpr uint32_t block_sizes[] = {16, 32, 64, 128};
static constexpr uint32_t bf_width = 16;
static constexpr float sparsities[] = {0.0, 0.20, 0.40, 0.60, 0.80, 0.90, 0.92, 0.96, 0.98, 0.99};

static constexpr uint32_t num_workers = 4;

static constexpr size_t data_size = 1UL << 25;

extern uint64_t computation_time;
extern uint64_t network_time;

int main() {
#if defined(DEBUGGING) || defined(VERBOSE)
    std::cerr << "Warning: it is recommended to run this experiment "
                 "without D=1 and without V=1" << std::endl;
#endif
    std::cout << "blocksize,sparsity,time" << std::endl;

    for (uint32_t i = 0; i != sizeof(block_sizes) / sizeof(uint32_t); ++i) {
        for (uint32_t j = 0; j != sizeof(sparsities) / sizeof(float); ++j) {
            Simulator s(num_workers, block_sizes[i], bf_width);
            s.generate_data(data_size, block_sizes[i], sparsities[j]);
            s.run();
            std::cout << block_sizes[i] << ","
                      << sparsities[j] << ","
                      << float(s.get_time()) / 1e6 << std::endl;
        }
    }
}
