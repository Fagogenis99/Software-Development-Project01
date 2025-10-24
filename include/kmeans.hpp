#pragma once
#include <vector>
#include <random>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include "dataset_io.hpp"  // Matrix { int n,d; std::vector<float> a; float* row(int); }

struct KMeansParams {
    int   k           = 50;     // number of clusters
    int   max_iters   = 50;     // Lloyd iterations
    float tol         = 1e-4f;  // stop when max centroid shift < tol
    int   seed        = 1;      // RNG seed
    bool  use_kmeanspp = true;  // k-means++ initialization
    int   train_subset = -1;    // if >0, use that many points to train (e.g. sqrt(n)); else use all
};

struct KMeansResult {
    Matrix centroids;           // k x d
    std::vector<int> assign;    // size = X.n : nearest centroid id for each point in X
    float final_sse = 0.0f;     // SSE on training set at convergence
    int   iters     = 0;        // iterations performed
};

// Train k-means on X. If train_subset > 0, fit on a subsample but return
// assignments for the FULL X using the final centroids.
KMeansResult kmeans_train(const Matrix& X, const KMeansParams& p);
