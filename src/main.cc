#include "simulator.h"

#include <iostream>

int main() {
    Simulator s(4);
    s.generate_data(1000000, 0.90);
    s.run();
    std::cout << "Done" << std::endl;
}
