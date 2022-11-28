#include "simulator.h"

#include <iostream>

int main() {
    Simulator s(2, 2048);
    s.generate_data(134217728, 0.999);
    s.run();
    std::cout << "Done" << std::endl;
}
