#pragma once
#include <random>
#include <vector>
#include <cmath>
#include <stdexcept>

namespace vutils {

// RNG — initialized via initRand; falls back to a default seed if not called.
inline std::mt19937& rng() {
    static std::mt19937 g(123456789u);
    return g;
}

inline void initRand(unsigned seed) {
    rng().seed(seed);
}

inline double normalRand() {
    static thread_local std::normal_distribution<double> dist(0.0, 1.0);
    return dist(rng());
}

inline double uniformRand(double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(rng());
}

inline double dotProduct(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) throw std::runtime_error("dotProduct: size mismatch");
    double s = 0.0;
    for (size_t i = 0; i < a.size(); ++i) s += static_cast<double>(a[i]) * b[i];
    return s;
}

inline double euclideanDistance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) throw std::runtime_error("euclideanDistance: size mismatch");
    double s = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double d = static_cast<double>(a[i]) - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}

} // namespace vutils
