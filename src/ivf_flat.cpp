#include "../include/ivf_flat.hpp"
#include <algorithm>
#include <limits>
#include <cmath>


static inline float l2_sq(const float* a, const float* b, int d) {
    float s = 0.0f;
    for (int i = 0; i < d; ++i) {
        float v = a[i] - b[i];
        s += v * v;
    }
    return s;
}

// Returns the indices of the nprobe closest centroids (in increasing distance)
static std::vector<int> top_nprobe_centroids(const Matrix& C, const float* q, int nprobe) {
    const int k = C.n;
    std::vector<std::pair<float,int>> dv; dv.reserve(k);
    for (int j = 0; j < k; ++j) {
        dv.emplace_back(l2_sq(q, C.row(j), C.d), j);
    }
    if (nprobe < k) {
        std::nth_element(dv.begin(), dv.begin() + nprobe, dv.end(),
                         [](const auto& A, const auto& B){ return A.first < B.first; });
        dv.resize(nprobe);
    }
    std::sort(dv.begin(), dv.end(),
              [](const auto& A, const auto& B){ return A.first < B.first; });

    std::vector<int> idx; idx.reserve((size_t)std::min(nprobe, k));
    for (const auto& p : dv) idx.push_back(p.second);
    return idx;
}

// ================== Index Construction ==================

IVFIndexFlat build_ivf_flat(const Matrix& base, int kclusters, int seed, int train_subset) {
    if (kclusters <= 0) throw std::runtime_error("ivf_flat: kclusters must be > 0");
    if (kclusters > base.n) throw std::runtime_error("ivf_flat: kclusters cannot exceed #points");

    KMeansParams kp;
    kp.k = kclusters;
    kp.max_iters = 50;
    kp.tol = 1e-4f;
    kp.seed = seed;
    kp.use_kmeanspp = true;
    kp.train_subset = (train_subset > 0 && train_subset < base.n) ? train_subset : -1;

    KMeansResult km = kmeans_train(base, kp);

    IVFIndexFlat ivf;
    ivf.centroids = std::move(km.centroids);
    ivf.lists.assign(kclusters, {});

    // Build inverted lists from the final assignments (for ALL points)
    for (int i = 0; i < base.n; ++i) {
        int c = km.assign[i];
        if ((unsigned)c >= ivf.lists.size())
            throw std::runtime_error("ivf_flat: invalid cluster id in assignments");
        ivf.lists[c].push_back(i);
    }
    return ivf;
}

// ================== Queries ==================

TopN ivf_flat_query_topN(const IVFIndexFlat& ivf,
                         const Matrix& base,
                         const float* q,
                         int nprobe,
                         int N) {
    TopN res;
    if (N <= 0) return res;
    if (ivf.centroids.n == 0) return res;

    nprobe = std::max(1, std::min(nprobe, ivf.centroids.n));

    // 1) Select the nprobe closest centroids
    std::vector<int> probes = top_nprobe_centroids(ivf.centroids, q, nprobe);

    // 2) Collect candidates from the corresponding inverted lists and compute distances
    std::vector<std::pair<float,int>> cand;
    for (int c : probes) {
        const auto& lst = ivf.lists[c];
        cand.reserve(cand.size() + lst.size());
        for (int id : lst) {
            float d2 = l2_sq(q, base.row(id), base.d);
            cand.emplace_back(d2, id);
        }
    }
    if (cand.empty()) return res;

    // 3) Keep the best N candidates
    if ((int)cand.size() > N) {
        std::nth_element(cand.begin(), cand.begin() + N, cand.end(),
                         [](const auto& A, const auto& B){ return A.first < B.first; });
        cand.resize(N);
    }
    std::sort(cand.begin(), cand.end(),
              [](const auto& A, const auto& B){ return A.first < B.first; });

    res.ids.reserve(cand.size());
    res.dists.reserve(cand.size());
    for (const auto& p : cand) {
        res.ids.push_back(p.second);
        res.dists.push_back(std::sqrt(p.first)); // return Euclidean distance (not squared)
    }
    return res;
}

std::vector<int> ivf_flat_query_range(const IVFIndexFlat& ivf,
                                      const Matrix& base,
                                      const float* q,
                                      int nprobe,
                                      float R) {
    std::vector<int> out;
    if (ivf.centroids.n == 0) return out;

    nprobe = std::max(1, std::min(nprobe, ivf.centroids.n));
    const float R2 = R * R;

    // 1) Select the nprobe closest centroids
    std::vector<int> probes = top_nprobe_centroids(ivf.centroids, q, nprobe);

    // 2) Scan only the corresponding lists and apply threshold on radius R
    for (int c : probes) {
        const auto& lst = ivf.lists[c];
        for (int id : lst) {
            float d2 = l2_sq(q, base.row(id), base.d);
            if (d2 <= R2) out.push_back(id);
        }
    }
    return out;
}
