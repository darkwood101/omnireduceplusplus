#include "simulator.h"

#include <iostream>

static uint32_t block_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
static float sparsities[] = {0.90, 0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99};

static constexpr size_t data_size = 134217728;

int main() {
    std::cout << "blocksize,sparsity,time" << std::endl;
    for (uint32_t i = 0; i != sizeof(block_sizes) / sizeof(uint32_t); ++i) {
        for (uint32_t j = 0; j != sizeof(sparsities) / sizeof(float); ++j) {
            Simulator s(4, block_sizes[i]);
            s.generate_data(data_size, sparsities[j]);
            s.run();
            std::cout << block_sizes[i] << "," << sparsities[j] << "," << s.get_time() << std::endl;
        }
    }
}
