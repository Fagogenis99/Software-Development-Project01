#ifndef VECTOR_UTILS_H
#define VECTOR_UTILS_H

#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <cmath>
#include <numeric>

namespace vutils{

    /*-------VECTOR----------*/
    //compute the dot product bwtween 2 vectors
    double dotProduct(const std::vector<float>& a, const std::vector<float>& b);

    //compute the euclidean distance between 2 vectors (L2)
    double euclideanDistance(const std::vector<float>& a, const std::vector<float>& b);

    //normalize a vector to unit length
    void normalize(std::vector<float>& a);

    //print a vector for dubugging purposes
    void printV(const std::vector<float>& a, const std::string& name = "");


    /*------RANDOM NUMBER HELPERS--*/
    //init a determinsitic random generator with a given seed
    void initRand(unsigned seed = 1);

    //generate a normally distributed random double(mean = 0, stddev = 1)
    double normalRand();

    //generate a uniformly distributed random double in [a, b)
    double uniformRand(double a, double b);

}

#endif //VECTOR_UTILS_H