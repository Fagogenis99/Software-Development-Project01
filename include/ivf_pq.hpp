#pragma once
#include <vector>
#include <cstdint>
#include "dataset_io.hpp"
#include "kmeans.hpp"

// Codebooks PQ: M υποχώροι, s=2^nbits κώδικες ανά υποχώρο
struct PQCodebooks {
    int M = 0;         // #subspaces
    int nbits = 8;     // bits per subspace
    int s = 256;       // codewords per subspace (2^nbits)
    int dsub = 0;      // dims per subspace (= d / M)
    std::vector<Matrix> C; // C[i]: s x dsub (centroids ανά υποχώρο)
};

// IVF+PQ index
struct IVFIndexPQ {
    Matrix centroids;                       // k x d (coarse)
    PQCodebooks pq;                         // shared codebooks
    std::vector<std::vector<int>> ids;      // inverted lists: ids[c]
    std::vector<std::vector<uint8_t>> codes;// inverted lists: flat codes[c] (packed M bytes per vector)
};

// Κατασκευή IVFPQ:
//  - coarse k-means (kclusters)
//  - εκπαίδευση PQ codebooks (M, nbits) πάνω σε residuals
//  - κωδικοποίηση residuals και χτίσιμο inverted lists
IVFIndexPQ build_ivf_pq(const Matrix& base,
                        int kclusters, int M, int nbits,
                        int seed, int train_subset);

// Top-N: ADC με LUTs στις nprobe λίστες
struct TopNPQ {
    std::vector<int> ids;
    std::vector<float> dists;
};

TopNPQ ivf_pq_query_topN(const IVFIndexPQ& ivf,
                       const Matrix& base, // μόνο για d και για πιθανή επαλήθευση, δεν διαβάζουμε floats
                       const float* q, int nprobe, int N);

// Range-R: επιστρέφει ids με approx απόσταση ≤ R
std::vector<int> ivf_pq_query_range(const IVFIndexPQ& ivf,
                                    const Matrix& base,
                                    const float* q, int nprobe, float R);
