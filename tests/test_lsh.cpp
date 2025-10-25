#include "lsh.h"
#include <iostream>

int main() {
    vutils::initRand(42);

    std::vector<std::vector<float>> data = {
        {1.0f, 2.0f}, {2.0f, 1.0f}, {8.0f, 9.0f}, {9.0f, 8.0f}
    };

    lsh::LSH index(2, 4, 5, 4.0);
    index.buildIndex(data);

    std::vector<float> query = {1.5f, 2.0f};

    auto approx = index.searchKNN(query, 1);
    std::cout << "Approx NN index: " << approx[0].first
              << " dist=" << approx[0].second << std::endl;

    auto range = index.searchRadius(query, 3.0);
    std::cout << "Neighbors in radius 3: ";
    for (int i : range) std::cout << i << " ";
    std::cout << std::endl;
}
