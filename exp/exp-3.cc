#include <iostream>
#include <cassert>

#include "simulator.h"
#include "utils.h"

bool isclose(float a, float b) {
    return std::abs(a - b) <= 1e-6;
}

#ifdef DEBUGGING
void do_test(uint32_t num_workers,
             uint32_t block_size,
             uint32_t bf_width,
             size_t data_sz,
             float sparsity) {
    std::cout << "Test params:" << std::endl;
    std::cout << "    Number of workers: " << num_workers << std::endl;
    std::cout << "    Block size (elements): " << block_size << std::endl;
    std::cout << "    Block fusion width: " << bf_width << std::endl;
    std::cout << "    Data size (elements): " << data_sz << std::endl;
    std::cout << "    Sparsity: " << sparsity << std::endl;

    Simulator s(num_workers, block_size, bf_width);
    s.generate_data(data_sz, sparsity);

    std::vector<float> result;
    result.resize(data_sz);
    std::fill(result.begin(), result.end(), 0);
    for (uint32_t i = 0; i != num_workers; ++i) {
        for (size_t j = 0; j != result.size(); ++j) {
            result[j] += s.workers_[i].gradients_[j];
        }
    }
    s.run();
    for (uint32_t i = 0; i != num_workers; ++i) {
        for (size_t j = 0; j != result.size(); ++j) {
            assert(isclose(s.workers_[i].gradients_[j], result[j]));
        }
    }

    std::cout << "PASS" << std::endl << std::endl;
}
#endif


int main() {
#ifndef DEBUGGING
    std::cerr << "This experiment must be compiled with D=1" << std::endl;
#else

    do_test(4, 64, 4, 1 << 20, 0.90);
    do_test(3, 128, 7, 1 << 18, 0.87);
    do_test(2, 8, 1, 1 << 18, 0.99);
    do_test(6, 7, 13, 700000, 0.999);
    do_test(6, 7, 13, 700000, 0.1);
    std::cout << "All tests passed" << std::endl;
#endif
    return 0;
}