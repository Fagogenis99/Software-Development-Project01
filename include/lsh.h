#ifndef LSH_H
#define LSH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <utility>
#include <cstdint>
#include "vector_utils.h"

//Locality Sensitive Hashing for approximate nearest neighbor search with L2 distance(Euclidean distance)
//implements h-and g- functions as described based on random projections and uniform offsets
//uses STL unordered_map as the hash table

namespace lsh {
    
    //class HFunction represents a single hash function h(p) = floor((v*p + t) / w)
    class HFunction {
        public:
            HFunction(int dim, double w); //constructor - dim: dimension of input vectors, w: bucket width
            int hash(const std::vector<float>& p) const; //compute h(p) for input vector p
        private:
            std::vector<float> v; //random projection vector N(0, 1)^d
            double t_off; //random offset in [0, w)
            double w_bucket; //bucket width
        };

    //class Gfunction represents a combination of k HFunctions  
    class GFunction {
        public:
            GFunction(int dim, double w, int k, int tableSize); //constructor
            int computeHashValue(const std::vector<float>& p, unsigned int& id) const; 
        private:
            std::vector<HFunction> hFunctions; //the k h functions of this g 
            std::vector<int> rand_coeffs; //random coefficients for combining h's into g
            int table_size; //size of the hash table (number of buckets)
            uint64_t M; //a large prime number for modulus

    };


    class LSH{
        public:
            LSH(int dim, int k, int L, double w, int tableSize = -1, unsigned seed = 1); //-1 for tableSize for auto  
            void buildIndex(const std::vector<std::vector<float>>& dataset);
            std::vector<std::pair<int, double>> searchKNN(const std::vector<float>& query, int N) const;
            std::vector<int> searchRadius(const std::vector<float>& query, double R) const;

        private:
            int dimension; //dimensionality of vectors
            int k_H; //k number of h-functions per g
            int L_Tables; // L number of hash tables
            double w_size; //window size
            int table_size;
            unsigned seed_;

            std::vector<GFunction> g_F; //G Functions (one per table)

            //each table: bucket -> list of (index, ID) pairs
            std::vector<std::unordered_map<int, std::vector<std::pair<unsigned, unsigned>>>> tables_;
           // std::vector<std::unordered_map<int, std::vector<unsigned>>> tables_; //hash tables
            std::vector<std::vector<float>> dataset; //stored dataset
    };
    
    }

#endif 