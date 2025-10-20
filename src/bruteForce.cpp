#include "bruteForce.h"
#include <algorithm> //for std::sort

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
   
}