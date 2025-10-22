#include "../include/kmeans.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

// ---------- small helpers ----------

static inline float l2_sq(const float* a, const float* b, int d) {
    float s = 0.0f;
    for (int i = 0; i < d; ++i) {
        float v = a[i] - b[i];
        s += v * v;
    }
    return s;
}

static inline int argmin_dist2(const Matrix& C, const float* x) {
    int best = 0;
    float bd = std::numeric_limits<float>::infinity();
    for (int j = 0; j < C.n; ++j) {
        float dj = l2_sq(C.row(j), x, C.d);
        if (dj < bd) { bd = dj; best = j; }
    }
    return best;
}

// Choose m distinct indices from [0..n-1] (without replacement).
static std::vector<int> choose_subset(int n, int m, std::mt19937& rng) {
    if (m >= n) {
        std::vector<int> idx(n); std::iota(idx.begin(), idx.end(), 0);
        return idx;
    }
    std::vector<int> idx(n); std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);
    idx.resize(m);
    return idx;
}

// k-means++ on a *training subset* X_sub (specified by indices train_idx into X).
// Returns centroids matrix (k x d).
static Matrix init_kmeanspp(const Matrix& X, const std::vector<int>& train_idx, int k, int seed) {
    const int d = X.d;
    if (k <= 0) throw std::runtime_error("kmeans: k must be > 0");
    if ((int)train_idx.size() < k) throw std::runtime_error("kmeans++: subset smaller than k");

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> uni0(0, (int)train_idx.size() - 1);

    Matrix C; C.n = k; C.d = d; C.a.assign((size_t)k * d, 0.0f);

    // 1) pick first center uniformly from subset
    int first_idx = train_idx[uni0(rng)];
    std::copy(X.row(first_idx), X.row(first_idx) + d, C.row(0));

    // Work arrays
    std::vector<float> D2(train_idx.size(), std::numeric_limits<float>::infinity());

    // 2..k) iterative sampling proportional to squared distance to nearest chosen center
    std::uniform_real_distribution<float> ur;
    for (int c = 1; c < k; ++c) {
        // update D2
        for (size_t i = 0; i < train_idx.size(); ++i) {
            const float* xi = X.row(train_idx[i]);
            float dist2 = l2_sq(xi, C.row(c - 1), d);
            if (dist2 < D2[i]) D2[i] = dist2;
        }
        // prefix sums
        double total = 0.0;
        for (float v : D2) total += (double)v;
        if (total <= 0.0) {
            // all identical -> pick random
            int ridx = train_idx[uni0(rng)];
            std::copy(X.row(ridx), X.row(ridx) + d, C.row(c));
            continue;
        }
        double r = ur(rng) * total;
        double acc = 0.0;
        size_t chosen = 0;
        for (; chosen < D2.size(); ++chosen) {
            acc += (double)D2[chosen];
            if (acc >= r) break;
        }
        if (chosen == D2.size()) chosen = D2.size() - 1;
        int picked = train_idx[ (int)chosen ];
        std::copy(X.row(picked), X.row(picked) + d, C.row(c));
    }
    return C;
}

// Random uniform initialization (fallback)
static Matrix init_random(const Matrix& X, const std::vector<int>& train_idx, int k, int seed) {
    const int d = X.d;
    if ((int)train_idx.size() < k) throw std::runtime_error("kmeans: subset smaller than k");
    std::mt19937 rng(seed);
    std::vector<int> picks = choose_subset((int)train_idx.size(), k, rng);
    Matrix C; C.n = k; C.d = d; C.a.assign((size_t)k * d, 0.0f);
    for (int c = 0; c < k; ++c) {
        int i = train_idx[picks[c]];
        std::copy(X.row(i), X.row(i) + d, C.row(c));
    }
    return C;
}

// Handle empty clusters by re-seeding to the point with largest current error (farthest from its centroid).
static void reseed_empties(Matrix& C, const Matrix& X, const std::vector<int>& train_idx,
                           const std::vector<int>& assign_train,
                           std::vector<int>& counts) {
    const int k = C.n, d = C.d;
    // compute per-point distance to assigned centroid
    int worst_i = -1; float worst_d2 = -1.0f;
    for (size_t t = 0; t < train_idx.size(); ++t) {
        int idx = train_idx[t];
        int c = assign_train[(int)t];
        float dist2 = l2_sq(X.row(idx), C.row(c), d);
        if (dist2 > worst_d2) { worst_d2 = dist2; worst_i = idx; }
    }
    for (int c = 0; c < k; ++c) {
        if (counts[c] == 0) {
            // re-seed to worst point
            std::copy(X.row(worst_i), X.row(worst_i) + d, C.row(c));
            counts[c] = 1; // will be adjusted on next iteration
        }
    }
}

// ---------- main API ----------

KMeansResult kmeans_train(const Matrix& X, const KMeansParams& p) {
    if (X.n <= 0 || X.d <= 0) throw std::runtime_error("kmeans: empty dataset");
    if (p.k <= 0) throw std::runtime_error("kmeans: k must be > 0");
    if (p.k > X.n) throw std::runtime_error("kmeans: k cannot exceed number of points");

    const int d = X.d;
    std::mt19937 rng(p.seed);

    // Build training subset indices
    std::vector<int> train_idx;
    if (p.train_subset > 0 && p.train_subset < X.n) {
        train_idx = choose_subset(X.n, p.train_subset, rng);
    } else {
        train_idx.resize(X.n);
        std::iota(train_idx.begin(), train_idx.end(), 0);
    }

    // Initialize centroids
    Matrix C = p.use_kmeanspp ? init_kmeanspp(X, train_idx, p.k, p.seed)
                              : init_random(X, train_idx, p.k, p.seed);

    // Buffers for training loop (on subset)
    std::vector<int>    assign_train(train_idx.size(), -1);
    std::vector<float>  sums((size_t)p.k * d, 0.0f);
    std::vector<int>    counts(p.k, 0);

    float prev_shift = std::numeric_limits<float>::infinity();
    int it = 0;
    float final_sse = 0.0f;

    for (; it < p.max_iters; ++it) {
        std::fill(sums.begin(), sums.end(), 0.0f);
        std::fill(counts.begin(), counts.end(), 0);

        // Assignment (on subset)
        final_sse = 0.0f;
        for (size_t t = 0; t < train_idx.size(); ++t) {
            int idx = train_idx[t];
            const float* xi = X.row(idx);

            // nearest centroid
            int best = 0;
            float bd = std::numeric_limits<float>::infinity();
            for (int c = 0; c < C.n; ++c) {
                float dc = l2_sq(xi, C.row(c), d);
                if (dc < bd) { bd = dc; best = c; }
            }
            assign_train[(int)t] = best;
            final_sse += bd;

            // accumulate
            float* srow = sums.data() + (size_t)best * d;
            for (int j = 0; j < d; ++j) srow[j] += xi[j];
            counts[best] += 1;
        }

        // Update centroids, measure shift
        float max_shift = 0.0f;
        for (int c = 0; c < C.n; ++c) {
            if (counts[c] == 0) continue; // handle later
            float inv = 1.0f / (float)counts[c];
            float* crow = C.row(c);
            float oldc;
            float shift_c = 0.0f;
            for (int j = 0; j < d; ++j) {
                oldc = crow[j];
                float newc = sums[(size_t)c * d + j] * inv;
                float diff = newc - oldc;
                shift_c += diff * diff;
                crow[j] = newc;
            }
            if (shift_c > max_shift) max_shift = shift_c;
        }

        // Re-seed empty clusters (if any)
        bool has_empty = false;
        for (int c = 0; c < C.n; ++c) if (counts[c] == 0) { has_empty = true; break; }
        if (has_empty) {
            reseed_empties(C, X, train_idx, assign_train, counts);
            // No early stop this round; continue to refine
            prev_shift = max_shift;
            continue;
        }

        // Early stop if shift small
        if (std::sqrt(max_shift) < p.tol) break;
        prev_shift = max_shift;
    }

    // Final assignment for the FULL dataset
    KMeansResult R;
    R.centroids = std::move(C);
    R.assign.resize(X.n);
    for (int i = 0; i < X.n; ++i) {
        R.assign[i] = argmin_dist2(R.centroids, X.row(i));
    }
    R.final_sse = final_sse;
    R.iters     = it + 1;
    return R;
}
