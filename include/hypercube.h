#ifndef HYPERCUBE_H
#define HYPERCUBE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "vector_utils.h"

//Hypercube ANN for Euclidean distance (L2)
//h_i(p) = floor((v_i * p + t_i)/w),   v_i ~ N(0,1)^d,  t_i ~ U(0,w)
//f_i: Z -> {0,1} (random but consistent per i)
//g(p) = [ f_1(h_1(p)), ..., f_k(h_k(p)) ]  (k-bit vertex)
//Build: For each point p: compute g(p) and insert its index into cube[g(p)]. 
//Query (KNN / Range):- Compute g(q), look in that vertex and in up to "probes" nearby vertices (small Hamming distance),
// examining at most `M` points total *  - Compute true distances for collected candidates and return top-N / within R

namespace cube {
    //L2 hash function (projection + shift + binning)
    class HFunction {
        public: 
            HFunction(int dim, double w); //constructor
            int hash(const std::vector<float>& p) const; //compute h(p)
        private:
            std::vector<float> v; //random projection Gaussian vector N(0, 1)^
            double t_off; //random offset in [0, w)
            double w_bucket; //bucket width
    };

    //hypercube index class
    class Hypercube {
        //dim: vector dimension
        //k: num of bits (cube dimension)
        //w: bucket width in h_i
        //M: max num of points to examine per query
        //probes: max num of vertices to visit per query (including home vertex)
        //seed: rng seed
        public:
            Hypercube(int dim, int k, double w, int M, int probes, unsigned seed = 1);

            //build index from dataset
            void buildIndex(const std::vector<std::vector<float>>& dataset);

            //approximate k-NN search
            std::vector<std::pair<int, double>>
            searchKNN(const std::vector<float>& query, int N) const;

            //range search within radius R
            std::vector<int>
            searchRadius(const std::vector<float>& query, double R) const;

        private:
            int dimension; //dimensionality of vectors
            int k_bits; //num of bits (cube dimension)
            double w_size; //bucket width in h_i
            int M_points; //max num of points to examine per query
            int probes_v; //max num of vertices to visit per query
            unsigned seed_; //rng seed


            std::vector<HFunction> h_F; //k H-Functions

            // f_i tables: for each i in [0..k), map h_i(p) -> {0,1}
            // mutable so we can lazily fill during const queries
            mutable std::vector<std::unordered_map<int,int>> f_tables;

            //cube: key = k-bit vertex striing, value = indices of points
            std::unordered_map<std::string, std::vector<unsigned>> cube_;

            //stored dataset
            std::vector<std::vector<float>> stored_dataset;

            //computing k-bit vertex for point p (g(p))
            std::string hashToVertex(const std::vector<float>& p) const;

            //generating up to limit vertices in increasing Hamming distance order
            //start: "home" (counts as 1st) / never exceeds "limit"
            std::vector<std::string> enumerateProbes(const std::string& home, int limit) const;

        };

    }

#endif //HYPERCUBE_H