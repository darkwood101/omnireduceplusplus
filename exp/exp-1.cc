#include "simulator.h"

#include <iostream>

static constexpr uint32_t block_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
static constexpr uint32_t bf_widths[] = {1, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
static constexpr float sparsities[] = {0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90,
                             0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99};

static constexpr size_t data_size = 1UL << 20;

int main() {
#if defined(DEBUGGING) || defined(VERBOSE)
    std::cerr << "Warning: it is recommended to run this experiment "
                 "without D=1 and without V=1" << std::endl;
#endif
    std::cout << "blocksize,sparsity,bfwidth,time" << std::endl;
    for (uint32_t i = 0; i != sizeof(block_sizes) / sizeof(uint32_t); ++i) {
        for (uint32_t j = 0; j != sizeof(sparsities) / sizeof(float); ++j) {
            for (uint32_t k = 0; k != sizeof(bf_widths) / sizeof(uint32_t); ++k) {
                Simulator s(4, block_sizes[i], bf_widths[i]);
                s.generate_data(data_size, sparsities[j]);
                s.run();
                std::cout
                    << block_sizes[i] << ","
                    << sparsities[j] << ","
                    << bf_widths[k] << ","
                    << s.get_time() << std::endl;
            }
        }
    }
}
