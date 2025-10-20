#ifndef BRUTEFORCE_H
#define BRUTEFORCE_H
#include <vector>
#include <utility>
#include "vector_utils.h"

//Brute force nearest neighbor search
//exact number of NN using L2
//it will be later used to evaluate the approximate NN methods as the ground truth

namespace brute {

    //Finding the N nearest neighbors of a query point q using brute force search
    //dataset : All poiints (vector of vectors)
    //query - The query vector
    //N - number of nearest neighbors to return
    //we return a Vector of pairs (index in dataset, distance)
    std::vector<std::pair<int, double>>
    knnSearch(const std::vector<std::vector<float>>& dataset, const std::vector<float> query, int N);
    

    //Finding all points within a given radius(range search)
    //dataset : all points
    //query - the query vector
    //R - search radius
    // we return a Vctor of indices of points within R dist
    std::vector<int>
    rangeSearch(const std::vector<std::vector<float>>& dataset, const std::vector<float> query, double R);

    }

#endif //BRUTEFORCE_H