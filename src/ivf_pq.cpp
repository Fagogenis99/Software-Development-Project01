#include "../include/ivf_pq.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

// ---------- helpers ----------

static inline float l2_sq(const float* a, const float* b, int d) {
    float s = 0.0f;
    for (int i = 0; i < d; ++i) { float v = a[i] - b[i]; s += v * v; }
    return s;
}

static std::vector<int> top_nprobe_centroids(const Matrix& C, const float* q, int nprobe) {
    const int k = C.n;
    std::vector<std::pair<float,int>> dv; dv.reserve(k);
    for (int j = 0; j < k; ++j) dv.emplace_back(l2_sq(q, C.row(j), C.d), j);
    if (nprobe < k) {
        std::nth_element(dv.begin(), dv.begin()+nprobe, dv.end(),
                         [](auto& A, auto& B){ return A.first < B.first; });
        dv.resize(nprobe);
    }
    std::sort(dv.begin(), dv.end(),
              [](auto& A, auto& B){ return A.first < B.first; });
    std::vector<int> idx; idx.reserve((size_t)std::min(nprobe, k));
    for (auto& p : dv) idx.push_back(p.second);
    return idx;
}

// subvector pointer for subspace i: [i*Dsub .. (i+1)*Dsub)
static inline const float* subvec(const float* x, int i, int dsub) {
    return x + i * dsub;
}

// argmin_h || r_i - C_i[h] ||^2  , C_i : s x dsub
static int nearest_code(const float* r_i, const Matrix& Ci) {
    int best = 0; float bd = std::numeric_limits<float>::infinity();
    for (int h = 0; h < Ci.n; ++h) {
        float d = l2_sq(r_i, Ci.row(h), Ci.d);
        if (d < bd) { bd = d; best = h; }
    }
    return best;
}

// build LUT[i][h] = || r_i(q) - C_i[h] ||^2
static void build_LUT(const PQCodebooks& pq, const float* rq, std::vector<float>& LUT) {
    LUT.assign((size_t)pq.M * pq.s, 0.0f);
    for (int i = 0; i < pq.M; ++i) {
        const float* r_i = subvec(rq, i, pq.dsub);
        const Matrix& Ci = pq.C[i]; // s x dsub
        float* rowLUT = LUT.data() + (size_t)i * pq.s;
        for (int h = 0; h < pq.s; ++h) {
            rowLUT[h] = l2_sq(r_i, Ci.row(h), pq.dsub);
        }
    }
}

// ---------- build ----------

IVFIndexPQ build_ivf_pq(const Matrix& base,
                        int kclusters, int M, int nbits,
                        int seed, int train_subset)
{
    if (kclusters <= 0) throw std::runtime_error("ivf_pq: kclusters must be > 0");
    if (kclusters > base.n) throw std::runtime_error("ivf_pq: kclusters > n");
    if (M <= 0) throw std::runtime_error("ivf_pq: M must be > 0");
    if (nbits <= 0 || nbits > 8) throw std::runtime_error("ivf_pq: nbits must be in [1,8] (uint8 codes)");

    if (base.d % M != 0) throw std::runtime_error("ivf_pq: d must be divisible by M");
    const int dsub = base.d / M;
    const int s = 1 << nbits;

    // 1) Coarse k-means
    KMeansParams kp;
    kp.k = kclusters;
    kp.max_iters = 50;
    kp.tol = 1e-4f;
    kp.seed = seed;
    kp.use_kmeanspp = true;
    kp.train_subset = (train_subset > 0 && train_subset < base.n) ? train_subset : -1;

    KMeansResult km = kmeans_train(base, kp);

    IVFIndexPQ ivf;
    ivf.centroids = std::move(km.centroids);
    ivf.ids.assign(kclusters, {});
    ivf.codes.assign(kclusters, {});
    ivf.pq.M = M; ivf.pq.nbits = nbits; ivf.pq.s = s; ivf.pq.dsub = dsub;
    ivf.pq.C.resize(M);

    // 2) Collect residuals for training codebooks (use a subset for speed)
    //    We take a subsample of ~sqrt(n) for training the codebooks
    int trainN = (int)std::sqrt((double)base.n);
    if (trainN < s) trainN = s;
    if (trainN > base.n) trainN = base.n;

    std::vector<int> idx(trainN);
    for (int i = 0; i < trainN; ++i) idx[i] = i; // simple prefix (could also randomize)

    // 3) Train codebooks per subspace
    for (int si = 0; si < M; ++si) {
        // Build residual matrix for subspace si: trainN x dsub
        Matrix RS; RS.n = trainN; RS.d = dsub; RS.a.assign((size_t)trainN * dsub, 0.0f);
        for (int t = 0; t < trainN; ++t) {
            int i = idx[t];
            int c = km.assign[i];
            const float* x = base.row(i);
            const float* cc = ivf.centroids.row(c);
            for (int j = 0; j < dsub; ++j) {
                RS.a[(size_t)t * dsub + j] = x[si*dsub + j] - cc[si*dsub + j];
            }
        }
        KMeansParams psub;
        psub.k = s;
        psub.max_iters = 50;
        psub.tol = 1e-4f;
        psub.seed = seed + 1234 + si; // different seed per subspace
        psub.use_kmeanspp = true;
        psub.train_subset = -1; // use RS entirely

        KMeansResult rsub = kmeans_train(RS, psub);
        ivf.pq.C[si] = std::move(rsub.centroids); // s x dsub
    }

    // 4) Encoding & inverted lists
    // For each point: find its coarse centroid c, residual r = x - c,
    // then per subspace si find the nearest code h and write it.
    for (int i = 0; i < base.n; ++i) {
        int c = km.assign[i];
        ivf.ids[c].push_back(i);
        auto& codes_c = ivf.codes[c];
        codes_c.reserve(codes_c.size() + (size_t)M);

        // Residual subspace by subspace (we don't store the full residual in memory)
        for (int si = 0; si < M; ++si) {
            // r_i = x_i - c_i (subspace)
            // find nearest h in codebook C[si]
            // For efficiency, compute argmin "in-place"
            int best = 0;
            float bd = std::numeric_limits<float>::infinity();

            for (int h = 0; h < s; ++h) {
                float d = 0.0f;
                const float* x = base.row(i) + si*dsub;
                const float* cc = ivf.centroids.row(c) + si*dsub;
                const float* ch = ivf.pq.C[si].row(h);
                for (int j = 0; j < dsub; ++j) {
                    float rj = (x[j] - cc[j]) - ch[j];
                    d += rj * rj;
                }
                if (d < bd) { bd = d; best = h; }
            }
            codes_c.push_back((uint8_t)best);
        }
    }

    return ivf;
}

// ---------- queries (ADC with LUTs) ----------

TopNPQ ivf_pq_query_topN(const IVFIndexPQ& ivf,
                         const Matrix& base,
                         const float* q, int nprobe, int N)
{
    TopNPQ res;
    if (N <= 0 || ivf.centroids.n == 0) return res;

    nprobe = std::max(1, std::min(nprobe, ivf.centroids.n));
    std::vector<int> probes = top_nprobe_centroids(ivf.centroids, q, nprobe);

    // We need a LUT per probed centroid (for the residual of q w.r.t. this centroid)
    std::vector<float> LUT; LUT.reserve((size_t)ivf.pq.M * ivf.pq.s);

    std::vector<std::pair<float,int>> cand;
    for (int c : probes) {
        // residual of q: rq = q - centroid[c]
        // Build LUT for this residual
        // (For optimization, you could compute rq once per subspace)
        std::vector<float> rq((size_t)base.d, 0.0f);
        const float* cc = ivf.centroids.row(c);
        for (int j = 0; j < base.d; ++j) rq[j] = q[j] - cc[j];

        build_LUT(ivf.pq, rq.data(), LUT);

        // Read the codes of inverted list c: packed M bytes per vector
        const auto& ids_c   = ivf.ids[c];
        const auto& codes_c = ivf.codes[c];
        const size_t stride = (size_t)ivf.pq.M;

        for (size_t k = 0; k < ids_c.size(); ++k) {
            const uint8_t* code = codes_c.data() + k * stride;
            // ADC distance
            float d = 0.0f;
            for (int si = 0; si < ivf.pq.M; ++si) {
                int h = (int)code[si];
                d += LUT[(size_t)si * ivf.pq.s + h];
            }
            cand.emplace_back(d, ids_c[k]);
        }
    }

    if (cand.empty()) return res;

    if ((int)cand.size() > N) {
        std::nth_element(cand.begin(), cand.begin()+N, cand.end(),
                         [](auto& A, auto& B){ return A.first < B.first; });
        cand.resize(N);
    }
    std::sort(cand.begin(), cand.end(),
              [](auto& A, auto& B){ return A.first < B.first; });

    res.ids.reserve(cand.size());
    res.dists.reserve(cand.size());
    for (auto& p : cand) { res.ids.push_back(p.second); res.dists.push_back(std::sqrt(p.first)); }
    return res;
}

std::vector<int> ivf_pq_query_range(const IVFIndexPQ& ivf,
                                    const Matrix& base,
                                    const float* q, int nprobe, float R)
{
    std::vector<int> out;
    if (ivf.centroids.n == 0) return out;

    nprobe = std::max(1, std::min(nprobe, ivf.centroids.n));
    std::vector<int> probes = top_nprobe_centroids(ivf.centroids, q, nprobe);

    std::vector<float> LUT; LUT.reserve((size_t)ivf.pq.M * ivf.pq.s);
    const float R2 = R * R;

    for (int c : probes) {
        std::vector<float> rq((size_t)base.d, 0.0f);
        const float* cc = ivf.centroids.row(c);
        for (int j = 0; j < base.d; ++j) rq[j] = q[j] - cc[j];

        build_LUT(ivf.pq, rq.data(), LUT);

        const auto& ids_c   = ivf.ids[c];
        const auto& codes_c = ivf.codes[c];
        const size_t stride = (size_t)ivf.pq.M;

        for (size_t k = 0; k < ids_c.size(); ++k) {
            const uint8_t* code = codes_c.data() + k * stride;
            float d = 0.0f;
            for (int si = 0; si < ivf.pq.M; ++si) {
                int h = (int)code[si];
                d += LUT[(size_t)si * ivf.pq.s + h];
            }
            if (d <= R2) out.push_back(ids_c[k]);
        }
    }
    return out;
}
