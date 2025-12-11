#ifndef BRUTEFORCE_H
#define BRUTEFORCE_H
#pragma once
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

    //NEW - Full knn graph for all points in dataset for project2 using knnSearch inside it
    std::vector<int>
    compute_knn_graph_all(const std::vector<std::vector<float>>& dataset, int k);

    //NEW - writing the knn graph into a binary file for pyhton extraction later 
    void save_knn_binary(const std::string& path, const std::vector<int>& knn_idx, int n, int k);
    }

#endif //BRUTEFORCE_H