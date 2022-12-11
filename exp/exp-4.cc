#include <iostream>
#include <cassert>

#include "simulator.h"
#include "utils.h"

static constexpr uint32_t block_size = 256;
static constexpr uint32_t bf_width = 16;
static constexpr float sparsities[] = {0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90,
                             0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99};

static constexpr uint32_t num_workers = 4;

static constexpr size_t data_size = 1UL << 27;

int main() {
#if defined(DEBUGGING) || defined(VERBOSE)
    std::cerr << "Warning: it is recommended to run this experiment "
                 "without D=1 and without V=1" << std::endl;
#endif
    std::cout << "sparsity,time" << std::endl;

    for (uint32_t j = 0; j != sizeof(sparsities) / sizeof(float); ++j) {
        Simulator s(num_workers, block_size, bf_width);
        s.generate_data(data_size, sparsities[j]);
        s.run();
        std::cout << sparsities[j] << "," << s.get_time() << std::endl;
    }
}
