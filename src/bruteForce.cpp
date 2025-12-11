#include "bruteForce.h"
#include <algorithm> //for std::sort
#include <fstream> //for file writing
#include <cstdint> //int32_t

namespace brute {

    std::vector<std::pair<int, double>>
    knnSearch(const std::vector<std::vector<float>>& dataset, const std::vector<float> query, int N){
        std::vector<std::pair<int, double>> distances; //pair of (index, distance)
        distances.reserve(dataset.size()); //reserve space
        
        //computing the distances from query to each one of the vectors of the dataset
        for(size_t i = 0; i < dataset.size(); ++i){
            double dist = vutils::euclideanDistance(dataset[i], query);
            distances.emplace_back(static_cast<int>(i), dist); //push back the pair
        }

        //sorting the distances in ascending order
        std::sort(distances.begin(), distances.end(),
                    [](auto& a, auto& b){
                        return a.second < b.second; }); //comparing based on distance only
        //keeping only the top N nearest neighbors
        if((int)distances.size() > N){
            distances.resize(N);
        }

        return distances;

    }

    std::vector<int>
    rangeSearch(const std::vector<std::vector<float>>& dataset, const std::vector<float> query, double R){

        std::vector<int> inRange;
        inRange.reserve(dataset.size()/10); //reserve some space--heuristic

        for(size_t i = 0; i < dataset.size(); ++i){
            double dist = vutils::euclideanDistance(dataset[i], query);
            if(dist <= R){
                inRange.push_back(static_cast<int>(i)); //add index to results
            }
        }
        return inRange;
    }

    std::vector<int>
    compute_knn_graph_all(const std::vector<std::vector<float>>& dataset, int k){
        const size_t n = dataset.size();
        if(n == 0 || k <= 0) return {};

        //keeping flatened matrix of size n*k
        std::vector<int> all_indices(n * k, -1); //initialized to -1

        for(size_t i = 0; i < n; i++){
            //we request k+1 neighbours because one of them will be the point itself
            auto neighbours = knnSearch(dataset, dataset[i], k + 1);

            std::vector<int> row;
            row.reserve(k); //reserve space for k neighbors

            for(const auto& p : neighbours){
                int idx = p.first; //index of neighbor
                if(idx == static_cast<int>(i)){
                    continue; //skip itself
                }
                row.push_back(idx);//add to row
                if((int)row.size() == k) break; //we have enough neighbors
            }

            while((int)row.size() < k){
                row.push_back(-1); //filling with -1 if not enough neighbors
            }

            for(int j = 0; j < k; j++){
                all_indices[i * k + j] = row[j];
            }
            
        }
        return all_indices; //returning the flattened knn graph
    }

    void save_knn_binary(const std::string& path,
                         const std::vector<int>& knn_idx,
                         int n, int K) {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) {
            throw std::runtime_error("Cannot open knn output file: " + path);
        }

        int32_t n32 = n;
        int32_t K32 = K;

        ofs.write(reinterpret_cast<const char*>(&n32), sizeof(int32_t));
        ofs.write(reinterpret_cast<const char*>(&K32), sizeof(int32_t));
        ofs.write(reinterpret_cast<const char*>(knn_idx.data()),
                  sizeof(int32_t) * n * K);
    }
   
}  
    //namespace brute