#include <iostream>
#include <cmath>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <algorithm>
#include "../include/bruteForce.h"
#include "../include/dataset_io.hpp"
#include "../include/vector_utils.h"
#include "../include/lsh.h"
#include "../include/hypercube.h"
#include "../include/ivf_flat.hpp"
#include "../include/lsh.h"
#include "../include/ivf_pq.hpp"


struct Config {
    // common
    std::string input_path;   // -d
    std::string query_path;   // -q
    std::string output_path = "results.txt"; // -o
    std::string type;         // -type mnist|sift
    int N = 1;                // -N
    double R = 2000.0;        // -R (MNIST default; overwrite if SIFT with 2.0)
    bool do_range = false;    // -range true|false
    int seed = 1;             // -seed

    // LSH
    bool use_lsh = false;
    int k = 4;                // -k
    int L = 5;                // -L
    double w = 4.0;           // -w

    // Hypercube
    bool use_hypercube = false;
    int kproj = 14;           // -kproj (d')
    int M = 10;               // -M (max candidates)
    int probes = 2;           // -probes

    // IVFFlat
    bool use_ivfflat = false;
    int kclusters = 50;       // -kclusters
    int nprobe = 5;           // -nprobe

    // IVFPQ
    bool use_ivfpq = false;
    int M_pq = 16;            // -M (number of sub-vectors for PQ)
    int nbits = 8;            // -nbits (2^nbits centroids per subspace)

    void finalize_defaults() {
        // R default depends on dataset type per assignment:
        // MNIST: R=2000, SIFT: R=2
        if (!type.empty()) {
            if (type == "sift" && R == 2000.0) R = 2.0;
        }
    }
};

static bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

static bool to_bool(const std::string& s) {
    if (iequals(s, "true") || s == "1") return true;
    if (iequals(s, "false") || s == "0") return false;
    throw std::runtime_error("Invalid boolean value: " + s + " (expected true|false|1|0)");
}

Config parse_args(int argc, char** argv) {
    Config cfg;
    if (argc < 2) throw std::runtime_error("Insufficient arguments. Use -d, -q, -type and a method flag (-lsh|-hypercube|-ivfflat|-ivfpq).");

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];

        auto need = [&](int more) {
            if (i + more >= argc) throw std::runtime_error("Missing value after " + k);
        };

        if (k == "-d") { need(1); cfg.input_path = argv[++i]; }
        else if (k == "-q") { need(1); cfg.query_path = argv[++i]; }
        else if (k == "-o") { need(1); cfg.output_path = argv[++i]; }
        else if (k == "-type") { need(1); cfg.type = argv[++i]; }
        else if (k == "-N") { need(1); cfg.N = std::stoi(argv[++i]); }
        else if (k == "-R") { need(1); cfg.R = std::stod(argv[++i]); }
        else if (k == "-range") { need(1); cfg.do_range = to_bool(argv[++i]); }
        else if (k == "-seed") { need(1); cfg.seed = std::stoi(argv[++i]); }

        // LSH
        else if (k == "-lsh") { cfg.use_lsh = true; }
        else if (k == "-k") { need(1); cfg.k = std::stoi(argv[++i]); }
        else if (k == "-L") { need(1); cfg.L = std::stoi(argv[++i]); }
        else if (k == "-w") { need(1); cfg.w = std::stod(argv[++i]); }

        // Hypercube
        else if (k == "-hypercube") { cfg.use_hypercube = true; }
        else if (k == "-kproj") { need(1); cfg.kproj = std::stoi(argv[++i]); }
        else if (k == "-M") {
            need(1);
            // M is used by both hypercube (max candidates) and IVFPQ (subspaces).
            // We'll set both; later the chosen method will use the relevant one.
            int val = std::stoi(argv[++i]);
            cfg.M = val;
            cfg.M_pq = val;
        }
        else if (k == "-probes") { need(1); cfg.probes = std::stoi(argv[++i]); }

        // IVFFlat
        else if (k == "-ivfflat") { cfg.use_ivfflat = true; }
        else if (k == "-kclusters") { need(1); cfg.kclusters = std::stoi(argv[++i]); }
        else if (k == "-nprobe") { need(1); cfg.nprobe = std::stoi(argv[++i]); }

        // IVFPQ
        else if (k == "-ivfpq") { cfg.use_ivfpq = true; }
        else if (k == "-nbits") { need(1); cfg.nbits = std::stoi(argv[++i]); }

        else {
            throw std::runtime_error("Unknown option: " + k);
        }
    }

    // method selection sanity
    int methods = (cfg.use_lsh?1:0) + (cfg.use_hypercube?1:0) + (cfg.use_ivfflat?1:0) + (cfg.use_ivfpq?1:0);
    if (methods != 1) throw std::runtime_error("Select exactly one method: -lsh | -hypercube | -ivfflat | -ivfpq");

    if (cfg.input_path.empty() || cfg.query_path.empty() || cfg.type.empty())
        throw std::runtime_error("Missing required arguments: -d <input> -q <query> -type <mnist|sift>");

    // finalize dataset-dependent defaults (R)
    cfg.finalize_defaults();

    if (!iequals(cfg.type, "mnist") && !iequals(cfg.type, "sift"))
        throw std::runtime_error("Invalid -type. Use mnist or sift.");

    return cfg;
}

// --- placeholders for algorithm entry points ---
void run_lsh(const Matrix& base, const Matrix& queries, const Config& cfg);
void run_hypercube(const Matrix& base, const Matrix& queries, const Config& cfg);
void run_ivfflat(const Matrix& base, const Matrix& queries, const Config& cfg);
void run_ivfpq(const Matrix& base, const Matrix& queries, const Config& cfg);

int main(int argc, char** argv) {
    try {
        Config cfg = parse_args(argc, argv);
        std::cerr << "Loading datasets...\n";

        Matrix base, queries;
        if (iequals(cfg.type, "mnist")) {
            base    = load_mnist_images(cfg.input_path, /*normalize=*/false);
            queries = load_mnist_images(cfg.query_path, /*normalize=*/false);
        } else {
            base    = load_fvecs(cfg.input_path);
            queries = load_fvecs(cfg.query_path);
        }

        if (base.d != queries.d) throw std::runtime_error("Dimension mismatch between base and query sets");

        std::cerr << "Loaded base n=" << base.n << " d=" << base.d
                  << " | queries n=" << queries.n << "\n";

        // dispatch
        if (cfg.use_lsh)        run_lsh(base, queries, cfg);
        else if (cfg.use_hypercube) run_hypercube(base, queries, cfg);
        else if (cfg.use_ivfflat)   run_ivfflat(base, queries, cfg);
        else if (cfg.use_ivfpq)     run_ivfpq(base, queries, cfg);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}

/*static std::pair<int, double> brute_force_nn(const Matrix& base, const std::vector<float>& query) {
    int bestIdx = -1;
    double bestDist = 1e18;
    for(int i = 0; i < base.n; ++i){
        std::vector<float> v(base.row(i), base.row(i) + base.d); // copy row i
        double dist = vutils::euclideanDistance(v, query);
        if(dist < bestDist){
            bestDist = dist;
            bestIdx = i;
        }
    }
    return {bestIdx, bestDist}; // return id and distance
}*/
//brute force--True top N neighbours
/*static std::vector<std::pair<int,double>>
brute_force_topN(const Matrix& base, const std::vector<float>& query, int N) {
    std::vector<std::pair<int,double>> all;
    all.reserve(base.n);
    for (int i = 0; i < base.n; ++i) {
        std::vector<float> v(base.row(i), base.row(i) + base.d);
        double dist = vutils::euclideanDistance(v, query);
        all.emplace_back(i, dist);
    }
    if ((int)all.size() > N)
        std::nth_element(all.begin(), all.begin()+N, all.end(),
                         [](auto& a, auto& b){ return a.second < b.second; });
    if ((int)all.size() > N) all.resize(N);
    std::sort(all.begin(), all.end(), [](auto& a, auto& b){ return a.second < b.second; });
    return all;
}*/

//compute recall@N
static double recall_at_N(const std::vector<std::pair<int,double>>& approx,
                          const std::vector<std::pair<int,double>>& truth) {
    std::unordered_set<int> trueSet;
    for (auto& p : truth) trueSet.insert(p.first);
    int found = 0;
    for (auto& p : approx) if (trueSet.count(p.first)) ++found;
    return truth.empty() ? 0.0 : (double)found / truth.size();
}

//compute approximation factor
static double af_top1(const std::vector<std::pair<int,double>>& approx,
                      const std::vector<std::pair<int,double>>& truth) {
    if (approx.empty() || truth.empty()) return 0.0;
    return approx[0].second / truth[0].second;
}

// --- dummy implementations just to compile now; replace with real ones -----
void run_lsh(const Matrix& base, const Matrix& queries, const Config& cfg){
    using namespace std::chrono;
    std::ofstream out(cfg.output_path);
    if(!out){ std::cerr << "[ERROR] Could not open output file: " << cfg.output_path << "\n"; return; }
   
    out << "LSH\n";

  //convertix Matrix to std::vector<std::vector<float>> only once
    std::vector<std::vector<float>> base_vecs(base.n);
    for(int i = 0; i < base.n; ++i)
        base_vecs[i] = std::vector<float>(base.row(i), base.row(i) + base.d);

  //building the lsh index
    lsh::LSH index(base.d, cfg.k, cfg.L, cfg.w, -1, cfg.seed);
    index.buildIndex(base_vecs);

    double sumAF = 0.0, sumRecall = 0.0, sumApprox = 0.0, sumTrue = 0.0;
    int Q = std::min(queries.n, 5); 

    for(int qi = 0; qi < Q; ++qi){
        std::vector<float> q(queries.row(qi), queries.row(qi) + queries.d);


        //aproximate search
        auto t0 = high_resolution_clock::now();
        auto approx = index.searchKNN(q, cfg.N);
        auto t1 = high_resolution_clock::now();
        double tApprox = duration_cast<microseconds>(t1 - t0).count() / 1000.0;

        //true - brute force search
        auto t2 = high_resolution_clock::now();
        auto truth = brute :: knnSearch(base_vecs, q, cfg.N);
        auto t3 = high_resolution_clock::now();
        double tTrue = duration_cast<microseconds>(t3 - t2).count() / 1000.0;

        //computing metrics with the above helper functions
        double AF = af_top1(approx, truth);
        double Recall = recall_at_N(approx, truth);

        sumAF += AF;
        sumRecall += Recall;
        sumApprox += tApprox;
        sumTrue += tTrue;

        //writting the query results now as shown in the exercise instructions
        out << "Query: " << qi << "\n";
        for (int i = 0; i < (int)approx.size(); ++i) {
            out << "Nearest neighbor-" << (i+1) << ": " << approx[i].first << "\n";
            out << "distanceApproximate: " << approx[i].second << "\n";
            out << "distanceTrue: " << truth[i].second << "\n";
        }

        if (cfg.do_range) {
            auto idsR = brute::rangeSearch(base_vecs, q, cfg.R);
            out << "R-near neighbors:\n";
            for (int id : idsR) out << id << "\n";
        }
    }

    // --- Summary ---
    double avgAF = sumAF / Q;
    double avgRecall = sumRecall / Q;
    double avgApprox = sumApprox / Q;
    double avgTrue = sumTrue / Q;
    double QPS = (Q) / (sumApprox / 1000.0);

    out << "Average AF: " << avgAF << "\n";
    out << "Recall@N: " << avgRecall << "\n";
    out << "QPS: " << QPS << "\n";
    out << "tApproximateAverage: " << avgApprox << "\n";
    out << "tTrueAverage: " << avgTrue << "\n";

    std::cout << "[LSH] Results saved to " << cfg.output_path << "\n";

}

void run_hypercube(const Matrix& base, const Matrix& queries, const Config& cfg){
    using namespace std::chrono;
    std::ofstream out(cfg.output_path);
    if(!out){ std::cerr << "[ERROR] Could not open output file: " << cfg.output_path << "\n"; return; }
   
    out << "Hypercube\n";

    //converting the dataset to vector of vectors
    std::vector<std::vector<float>> base_vecs(base.n);
    for(int i = 0; i < base.n; ++i)
        base_vecs[i] = std::vector<float>(base.row(i), base.row(i) + base.d);

    cube::Hypercube hc(base.d, cfg.kproj, cfg.w, cfg.M, cfg.probes, cfg.seed);
    hc.buildIndex(base_vecs);

    double sumAF = 0.0, sumRecall = 0.0, sumTrue = 0.0, sumApprox = 0.0;
    int Q = std::min(queries.n, 5);

    for(int qi = 0; qi < Q; ++qi){
        std::vector<float> q(queries.row(qi), queries.row(qi) + queries.d);

        //approximate
        auto t0 = high_resolution_clock::now();
        auto approx = hc.searchKNN(q, cfg.N);
        auto t1 = high_resolution_clock::now();
        auto tApprox = duration_cast<microseconds>(t1 - t0).count() / 1000.0;

         //true
        auto t2 = high_resolution_clock::now();
        auto truth = brute::knnSearch(base_vecs, q, cfg.N);
        auto t3 = high_resolution_clock::now();
        auto tTrue = duration_cast<microseconds>(t3 - t2).count() / 1000.0;

        //computing the metrics 
        double AF = af_top1(approx, truth);
        double Recall = recall_at_N(approx, truth);

        sumAF += AF;
        sumRecall += Recall;
        sumApprox +=tApprox;
        sumTrue += tTrue;

        out << "Query: " << qi << "\n";
        for (int i = 0; i < (int)approx.size(); ++i) {
            out << "Nearest neighbor-" << (i+1) << ": " << approx[i].first << "\n";
            out << "distanceApproximate: " << approx[i].second << "\n";
            out << "distanceTrue: " << truth[i].second << "\n";
        }

        if (cfg.do_range) {
            auto idsR = brute::rangeSearch(base_vecs, q, cfg.R);
            out << "R-near neighbors:\n";
            for (int id : idsR) out << id << "\n";
        }
    }

    double avgAF = sumAF / Q;
    double avgRecall = sumRecall / Q;
    double avgApprox = sumApprox / Q;
    double avgTrue = sumTrue / Q;
    double QPS = (Q) / (sumApprox / 1000.0);

    out << "Average AF: " << avgAF << "\n";
    out << "Recall@N: " << avgRecall << "\n";
    out << "QPS: " << QPS << "\n";
    out << "tApproximateAverage: " << avgApprox << "\n";
    out << "tTrueAverage: " << avgTrue << "\n";

    out.close();

    std::cout << "[Hypercube] Results saved to " << cfg.output_path << "\n";


}

/*void run_ivfflat(const Matrix& base, const Matrix& queries, const Config& cfg){
    // 1) Build IVF index (coarse k-means + inverted lists)
    int train_subset = (int)std::sqrt((double)base.n);  // good default
    auto ivf = build_ivf_flat(base, cfg.kclusters, cfg.seed, train_subset);
    std::cout << "IVF built: k=" << cfg.kclusters
              << ", avg list size ≈ " << (double)base.n / std::max(1, ivf.centroids.n) << "\n";

    // 2) Quick smoke test: run a few queries (no metrics yet)
    const int show = std::min(3, queries.n);
    for (int i = 0; i < show; ++i) {
        if (!cfg.do_range) {
            auto ans = ivf_flat_query_topN(ivf, base, queries.row(i), cfg.nprobe, cfg.N);
            std::cout << "q" << i << " → got " << ans.ids.size()
                      << " | nn id=" << (ans.ids.empty() ? -1 : ans.ids[0])
                      << " dist=" << (ans.dists.empty() ? -1.0f : ans.dists[0]) << "\n";
        } else {
            auto ids = ivf_flat_query_range(ivf, base, queries.row(i), cfg.nprobe, (float)cfg.R);
            std::cout << "q" << i << " → " << ids.size() << " ids within R\n";
        }
    }
}*/

void run_ivfflat(const Matrix& base, const Matrix& queries, const Config& cfg) {
    using namespace std::chrono;
    std::cout << "[IVFFlat] Building index...\n";

    int train_subset = (int)std::sqrt((double)base.n);
    auto ivf = build_ivf_flat(base, cfg.kclusters, cfg.seed, train_subset);

    std::cout << "IVF built: k=" << cfg.kclusters
              << ", avg list size ≈ " << (double)base.n / std::max(1, ivf.centroids.n) << "\n";

    // Convert base Matrix to std::vector<std::vector<float>> for brute-force search
    std::vector<std::vector<float>> base_vecs(base.n);
    for (int i = 0; i < base.n; ++i)
        base_vecs[i] = std::vector<float>(base.row(i), base.row(i) + base.d);

    double total_recall = 0.0, total_af = 0.0;
    double total_tApprox = 0.0, total_tTrue = 0.0;

    const int Q = std::min(queries.n, 5); // test on 5 queries for speed
    for (int qi = 0; qi < Q; ++qi) {
        std::vector<float> q(queries.row(qi), queries.row(qi) + queries.d);

        // --- Approximate search ---
        auto t0 = high_resolution_clock::now();
        auto ans = ivf_flat_query_topN(ivf, base, q.data(), cfg.nprobe, cfg.N);
        auto t1 = high_resolution_clock::now();
        double tApprox = duration_cast<microseconds>(t1 - t0).count() / 1000.0;
        total_tApprox += tApprox;

        // --- True NN via brute force ---
        auto t2 = high_resolution_clock::now();
        auto truth = brute::knnSearch(base_vecs, q, cfg.N);
        auto t3 = high_resolution_clock::now();
        double tTrue = duration_cast<microseconds>(t3 - t2).count() / 1000.0;
        total_tTrue += tTrue;

        // --- Metrics for this query ---
        double af = (ans.dists.empty() || truth.empty()) ? 0.0 : ans.dists[0] / truth[0].second;
        double recall = (ans.ids.empty() || truth.empty()) ? 0.0 :
                        ((ans.ids[0] == truth[0].first) ? 1.0 : 0.0);

        total_af += af;
        total_recall += recall;

        std::cout << "q" << qi
                  << " NN: id=" << (ans.ids.empty() ? -1 : ans.ids[0])
                  << " dApprox=" << (ans.dists.empty() ? -1.0 : ans.dists[0])
                  << " dTrue=" << (truth.empty() ? -1.0 : truth[0].second)
                  << " AF=" << af
                  << " Recall=" << recall
                  << " tApprox=" << tApprox << "ms"
                  << " tTrue=" << tTrue << "ms\n";
    }

    // --- Compute averages ---
    double avg_recall = total_recall / Q;
    double avg_af = total_af / Q;
    double avg_tApprox = total_tApprox / Q;
    double avg_tTrue = total_tTrue / Q;
    double qps = (avg_tApprox > 0) ? (1000.0 / avg_tApprox) : 0.0;

    std::cout << "\n[IVFFlat Summary]\n"
              << "  Average AF: " << avg_af << "\n"
              << "  Recall@N: " << avg_recall << "\n"
              << "  QPS: " << qps << "\n"
              << "  tApproximateAverage: " << avg_tApprox << "\n"
              << "  tTrueAverage: " << avg_tTrue << "\n";
}

void run_ivfpq(const Matrix& base, const Matrix& queries, const Config& cfg) {
    using namespace std;
    using namespace std::chrono;

    // 1) Build IVFPQ index (coarse k-means + product quantizer)
    int train_subset = (int)std::sqrt((double)base.n);
    if (train_subset < 1000) train_subset = std::min(1000, base.n);

    auto ivf = build_ivf_pq(base, cfg.kclusters, cfg.M_pq, cfg.nbits, cfg.seed, train_subset);

    cout << "IVFPQ built: k=" << cfg.kclusters
         << ", M=" << cfg.M_pq
         << ", nbits=" << cfg.nbits
         << ", dsub=" << (base.d / cfg.M_pq)
         << ", avg list size ≈ " << (double)base.n / std::max(1, ivf.centroids.n)
         << "\n";

    // 2) Quick smoke test — first few queries
    const int show = std::min(3, queries.n);
    for (int i = 0; i < show; ++i) {
        if (!cfg.do_range) {
            auto ans = ivf_pq_query_topN(ivf, base, queries.row(i), cfg.nprobe, cfg.N);
            cout << "q" << i << " → got " << ans.ids.size()
                 << " | nn id=" << (ans.ids.empty() ? -1 : ans.ids[0])
                 << " dist=" << (ans.dists.empty() ? -1.0f : ans.dists[0]) << "\n";
        } else {
            auto ids = ivf_pq_query_range(ivf, base, queries.row(i), cfg.nprobe, (float)cfg.R);
            cout << "q" << i << " → " << ids.size() << " ids within R\n";
        }
    }
    cout << "Tested " << show << " queries.\n";
    cout << "Use -N <int> or -R <float> and -range true|false to change search mode.\n";

    // 3) === Evaluation for scripts (prints the lines your grep expects) ===
    // Convert base to vector<vector<float>> for brute force
    std::vector<std::vector<float>> base_vecs(base.n);
    for (int i = 0; i < base.n; ++i)
        base_vecs[i] = std::vector<float>(base.row(i), base.row(i) + base.d);

    double total_recall = 0.0, total_af = 0.0;
    double total_tApprox = 0.0, total_tTrue = 0.0;

    const int Q = std::min(queries.n, 5); // evaluate on 5 queries (same as others)
    for (int qi = 0; qi < Q; ++qi) {
        std::vector<float> q(queries.row(qi), queries.row(qi) + queries.d);

        // Approximate
        auto t0 = high_resolution_clock::now();
        auto ans = ivf_pq_query_topN(ivf, base, q.data(), cfg.nprobe, cfg.N);
        auto t1 = high_resolution_clock::now();
        double tApprox = duration_cast<microseconds>(t1 - t0).count() / 1000.0;
        total_tApprox += tApprox;

        // True (brute)
        auto t2 = high_resolution_clock::now();
        auto truth = brute::knnSearch(base_vecs, q, cfg.N);
        auto t3 = high_resolution_clock::now();
        double tTrue = duration_cast<microseconds>(t3 - t2).count() / 1000.0;
        total_tTrue += tTrue;

        // Metrics
        double af = (ans.dists.empty() || truth.empty()) ? 0.0 : ans.dists[0] / truth[0].second;
        double recall = (ans.ids.empty() || truth.empty()) ? 0.0 :
                        ((ans.ids[0] == truth[0].first) ? 1.0 : 0.0);

        total_af += af;
        total_recall += recall;
    }

    double avg_recall = total_recall / Q;
    double avg_af = total_af / Q;
    double avg_tApprox = total_tApprox / Q;
    double qps = (avg_tApprox > 0) ? (1000.0 / avg_tApprox) : 0.0;

    // EXACT strings used by your scripts:
    cout << "Average AF: " << avg_af << "\n";
    cout << "Recall@N: " << avg_recall << "\n";
    cout << "QPS: " << qps << "\n";
    cout << "tApproximateAverage: " << avg_tApprox << "\n";
}

