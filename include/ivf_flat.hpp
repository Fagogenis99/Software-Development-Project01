#pragma once
#include <vector>
#include "dataset_io.hpp"  // Matrix { int n,d; std::vector<float> a; float* row(int); }
#include "kmeans.hpp"      // KMeansParams, KMeansResult, kmeans_train

// Δομή του IVFFlat index: coarse centroids + inverted lists με IDs βάσης
struct IVFIndexFlat {
    Matrix centroids;                       // k × d
    std::vector<std::vector<int>> lists;    // lists[j] = IDs σημείων στο cluster j
};

// Κατασκευή του IVFFlat:
//  - Εκπαιδεύει k-means (με k-means++) στο base (προαιρετικά σε train_subset ≈ sqrt(n))
//  - Δημιουργεί inverted lists χρησιμοποιώντας τις τελικές αναθέσεις
IVFIndexFlat build_ivf_flat(const Matrix& base, int kclusters, int seed, int train_subset);

// Αποτέλεσμα top-N: IDs + αποστάσεις (αύξουσα σειρά)
struct TopN {
    std::vector<int> ids;      // μέγεθος ≤ N
    std::vector<float> dists;  // ίδιου μεγέθους με ids (ευκλείδειες αποστάσεις)
};

// Ερώτημα top-N: 
//  - Βρες τα nprobe κοντινότερα centroids
//  - Σάρωσε "επίπεδα" (flat) τις αντίστοιχες λίστες και επέστρεψε τα N κοντινότερα σημεία
TopN ivf_flat_query_topN(const IVFIndexFlat& ivf,
                         const Matrix& base,
                         const float* q,
                         int nprobe,
                         int N);

// Ερώτημα range-R:
//  - Όπως πάνω, αλλά επιστρέφει ΟΛΑ τα IDs από τις nprobe λίστες με ||q - x|| ≤ R
std::vector<int> ivf_flat_query_range(const IVFIndexFlat& ivf,
                                      const Matrix& base,
                                      const float* q,
                                      int nprobe,
                                      float R);
