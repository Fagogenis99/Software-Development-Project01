#include <iostream>
#include <cmath>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <cctype>
#include "../include/dataset_io.hpp"
#include "../include/ivf_flat.hpp"


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

// --- dummy implementations just to compile now; replace with real ones -----
void run_lsh(const Matrix&, const Matrix&, const Config&) {
    std::cout << "LSH not implemented yet.\n";
}
void run_hypercube(const Matrix&, const Matrix&, const Config&) {
    std::cout << "Hypercube not implemented yet.\n";
}
void run_ivfflat(const Matrix& base, const Matrix& queries, const Config& cfg) {
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
}
void run_ivfpq(const Matrix&, const Matrix&, const Config&) {
    std::cout << "IVFPQ not implemented yet.\n";
}
