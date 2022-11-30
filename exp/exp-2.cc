#include "simulator.h"

#include <iostream>

int main() {
    Simulator s(2, 16, 8);
    s.generate_data(1048576, 0.90);
    s.run();
    std::cout << "Done" << std::endl;
}
