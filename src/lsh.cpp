#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <unordered_set>

#include "lsh.h"

namespace lsh {

    /*----------H Function--------*/
    HFunction::HFunction(int dim, double w) : v(dim), w_bucket(w) {
        for(int i = 0; i < dim; ++i)
            v[i] = static_cast<float>(vutils::normalRand()); //Gaussian N(0,1) --maybe cahnge <float> to <double> ??
        t_off = vutils::uniformRand(0.0, w_bucket); //uniform in [0,w)
    }

    int HFunction::hash(const std::vector<float>& p) const {
        double projection = vutils::dotProduct(v, p); //v*p
        return static_cast<int>(std::floor((projection + t_off) / w_bucket));
    }

    /*----------G Function--------*/
    GFunction::GFunction(int dim, double w, int k, int tableSize) : hFunctions{}, rand_coeffs(k), table_size(tableSize), M(4294967291ULL) { //ULL sufix for unsigned long long
        hFunctions.reserve(k); //reserve space for k HFunctions
        for(int i = 0; i < k; ++i)
            hFunctions.emplace_back(dim, w); //creating HFunction and addin it to the vector
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(1, 1000000000);

        for(int i = 0; i < k; ++i)
            rand_coeffs[i] = dist(rng); //random coeffefficients for combining h's into g
    }

    int GFunction::computeHashValue(const std::vector<float>& p, unsigned int& id) const {
        long long sum = 0;
        for(size_t i = 0; i < hFunctions.size(); ++i){
            int hi = hFunctions[i].hash(p); //compute h_i(p)
            sum += (static_cast<long long>(rand_coeffs[i]) * hi) % M; 
        }

        sum = ((sum % M) + M) % M; //ensure non-negative
        id = static_cast<unsigned int>(sum);
        return static_cast<int>(id % static_cast<unsigned int>(table_size)); 
    }

    /*----------LSH-------*/

    LSH::LSH(int dim, int k, int L, double w, int tableSize, unsigned seed)
        : dimension(dim), k_H(k), L_Tables(L), w_size(w), table_size(tableSize), seed_(seed) 
    {
        g_F.reserve(L_Tables); //reserve space for L GFunctions
        tables_.reserve(L_Tables); //reserve space for L hash tables
        vutils::initRand(seed_); //initializing global random engine with seed

        for(int i = 0; i < L_Tables; ++i)
            g_F.emplace_back(dimension, w_size, k_H, table_size); //creating GFunction and adding it to the vector
    }

    void LSH::buildIndex(const std::vector<std::vector<float>>& dataset) {
        this->dataset = dataset; //storing dataset
        int n = static_cast<int>(dataset.size()); 

        //if table_size not set, use heuristic n/8 (>= 1)
        if(table_size <= 0)
            table_size = std::max(1, n / 8); //heuristic

        //initializing hash tables
        tables_.clear(); //clear any existing tables
        tables_.reserve(L_Tables);
        for(int i = 0; i < L_Tables; ++i){
            tables_.emplace_back(); //creating empty hash table
            tables_[i].reserve(table_size); 
        }

        //inserting each vector into all L tables
        for(size_t index = 0; index < dataset.size(); ++index){
            for(int j = 0; j < L_Tables; ++j){
                unsigned int id;
                int bucket = g_F[j].computeHashValue(dataset[index], id); //compute g_j(p) to get bucket
                tables_[j][bucket].emplace_back(index, id); //insert (index, ID) pair into the bucket
            }
        }

        std::cout << "LSH Index Built: " << n << " Vectors, " << L_Tables << "Tables, Table Size = " << table_size << std::endl;

    }

    std::vector<std::pair<int, double>>
    LSH::searchKNN(const std::vector<float>& query, int N) const {
        std::unordered_set<unsigned> candidates; //to avoid duplicates

        for(int i = 0; i <L_Tables; ++i){
            unsigned int query_id;
            int bucket = g_F[i].computeHashValue(query, query_id); //geting bucket

            auto bucket_it = tables_[i].find(bucket); //lloking up the bucket in the current table
            if(bucket_it == tables_[i].end()) continue; //bucket not found, continue

            //queuerying trick - only consider points with same ID
            for(auto& entry: bucket_it -> second){
                if(entry.second == query_id)
                    candidates.insert(entry.first); //adding index to candidates
            }
        }

        if(candidates.empty()){
            for(size_t i = 0; i < dataset.size(); ++i)
                candidates.insert(i); //if no candidates found, consider all dataset points
        }

        std::vector<std::pair<int, double>> results; //to store (index, distance) pairs
        results.reserve(candidates.size()); //reserve space

        for(auto index : candidates){
            double dist = vutils::euclideanDistance(query, dataset[index]);
            results.emplace_back(static_cast<int>(index), dist); //storing index and distance
        }

        if (static_cast<int>(results.size()) > N)
        std::nth_element(results.begin(), results.begin() + N, results.end(),
                         [](auto& a, auto& b){ return a.second < b.second; });

        std::sort(results.begin(), results.begin() + std::min(N, (int)results.size()),
                [](auto& a, auto& b){ return a.second < b.second; });

        if((int)results.size() > N)
            results.resize(N);

        return results;
    }

    std::vector<int>
    LSH::searchRadius(const std::vector<float>& query, double R) const {
        std::unordered_set<unsigned> candidates; //to avoid duplicates

        for(int i = 0; i <L_Tables; ++i){
            unsigned int query_id;
            int bucket = g_F[i].computeHashValue(query, query_id); //geting bucket

            auto bucket_it = tables_[i].find(bucket); //lloking up the bucket in the current table
            if(bucket_it == tables_[i].end()) continue; //bucket not found, continue

            //queuerying trick - only consider points with same ID
            for(auto& entry: bucket_it -> second){
                if(entry.second == query_id)
                    candidates.insert(entry.first); //adding index to candidates
        }
    }

    std::vector<int> neighbours;
    neighbours.reserve(candidates.size());

    for(auto index : candidates){
        double dist = vutils::euclideanDistance(query, dataset[index]);
        if(dist <= R)
            neighbours.push_back(static_cast<int>(index));  //storing index of neighbour within radius R
    }

    return neighbours;
    }

}