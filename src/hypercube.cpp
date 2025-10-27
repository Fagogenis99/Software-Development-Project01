#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <queue>

#include "hypercube.h"

namespace cube {

    /*-----HFunction----*/

    HFunction::HFunction(int dim, double w) : v(dim), w_bucket(w) {
        for(int i = 0; i < dim; ++i) //random Gaussian vector N(0,1)^d
            v[i] = static_cast<float>(vutils::normalRand());
        t_off = vutils::uniformRand(0.0, w_bucket); //random offset in [0,w)
    }

    int HFunction::hash(const std::vector<float>& p) const {
        const double projection = vutils::dotProduct(v, p); //v*p
        return static_cast<int>(std::floor((projection + t_off) / w_bucket)); 
    }

    /*------Hypercube------*/

    Hypercube::Hypercube(int dim, int k, double w, int M, int probes, unsigned seed)
        : dimension(dim), k_bits(k), w_size(w), M_points(M), probes_v(probes), seed_(seed)
    {
        vutils::initRand(seed_);
        h_F.reserve(k_bits); //reserving space for k HFunctions
        for(int i = 0; i < k_bits; ++i)
            h_F.emplace_back(dimension, w_size); //creating HFunction and adding it to the vector
        
        f_tables.resize(k_bits); //resizing f_tables to hold k maps
    }

    void Hypercube::buildIndex(const std::vector<std::vector<float>>& dataset) {
        stored_dataset = dataset; //storing dataset
        cube_.clear(); //clearing cube
        cube_.reserve(std::max(1, static_cast<int>(stored_dataset.size())));

        for(size_t index = 0; index < stored_dataset.size(); ++index){
            const std::string vertex = hashToVertex(stored_dataset[index]); //computing k-bit vertex for point
            cube_[vertex].push_back(static_cast<unsigned>(index)); //inserting index into the corresponding vertex bucket
        }
    }

    //computing k-bit vertex for point p (g(p))
    std::string Hypercube::hashToVertex(const std::vector<float>& p) const {
        std::string bits;
        bits.resize(k_bits); //resizing to k bits

        for(int i = 0; i < k_bits; ++i){
            int h_i = h_F[i].hash(p); //computing h_i(p)

            //f_i -> assign 0 or 1 wi probability 0.5 each
            auto &f_i = f_tables[i]; //reference to f_i table
            auto it = f_i.find(h_i); //looking for h_i in f_i table
            int bit;
            if(it == f_i.end()){
                bit = vutils::uniformRand(0.0, 1.0) < 0.5 ? 0 : 1; //randomly assign 0 or 1
                f_i.emplace(h_i, bit); //inserting into f_i table
            } else {
                bit = it->second; //retrieve existing bit
            }
            bits[i] = static_cast<char>('0' + bit); //setting bit in vertex string
        }

        return bits;

    }

    std::vector<std::string>
    Hypercube::enumerateProbes(const std::string& home, int limit) const { //generatin up to a certain threshold
        std::vector<std::string> order; 
        order.reserve(std::max(1, limit));
        order.push_back(home); //adding home vertex first

        if(limit <= 1) return order; //if limit is 1, return only home

        const int n = static_cast<int>(home.size()); 

        //distance 1 
        for(int i = 0; i < n && static_cast<int>(order.size()) < limit; ++i){ //with this loop we enumerate neighbours of the “home” vertex in the Hamming cube
            std::string str = home;
            str[i] = (str[i] == '0') ? '1' : '0';
            order.push_back(std::move(str));
        }
        if(static_cast<int>(order.size()) >= limit) return order; //reached limit case

         //distance 2 -> pair flips
         for(int i = 0; i < n && static_cast<int>(order.size()) < limit; ++i){
            for(int j = i+1; j < n && static_cast<int>(order.size()) < limit; ++i){
                std::string str = home;
                str[i] = (str[i] == '0') ? '1' : '0';
                str[j] = (str[j] == '0') ? '1' : '0';
                order.push_back(std::move(str));
            }
         }

         if(static_cast<int>(order.size()) >= limit) return order; //reached limit case

         //distance 3+ / general case -> BFS-like approach
         std::unordered_set<std::string>
         visited(order.begin(), order.end()); //to avoid duplicates
         std::queue<std::string> q;
         q.push(home); //starting from home
         while(!q.empty() && static_cast<int>(order.size()) < limit){
            std::string current = q.front();//first in queue
            q.pop(); //removing from queue
            for(int i = 0; i < n && static_cast<int>(order.size()) < limit; ++i){
                std::string neighbour = current;
                neighbour[i] = (neighbour[i] == '0') ? '1' : '0';
                if(visited.insert(neighbour).second){ //if not visited 
                    order.push_back(neighbour); //adding neighbour to order
                    q.push(std::move(neighbour)); //adding to queue for further exploration
                }
            }    
        }

        if(static_cast<int>(order.size()) > limit) order.resize(limit); //trimming to limit if exceeded
        return order;
    }    


    std::vector<std::pair<int, double>>
    Hypercube::searchKNN(const std::vector<float>& query, int N) const {
        const std::string query_vertex = hashToVertex(query); //computing home vertex for query
        const std::vector<std::string> toVisit = enumerateProbes(query_vertex, std::max(1, probes_v)); //getting vertices to visit

        std::unordered_set<unsigned> candidates; //to avoid duplicates
        candidates.reserve(static_cast<int>(std::min(M_points, static_cast<int>(stored_dataset.size())))); //reserving space for candidates 

        for(const auto& vertex: toVisit){
            auto it = cube_.find(vertex); //looking for vertex in cube
            if(it == cube_.end()) continue; //vertex not found, continue
            for(auto index : it ->second){
                if(static_cast<int>(candidates.size()) >= M_points) break;
                candidates.insert(index); //adding index to candidates
            }

            if(static_cast<int>(candidates.size()) >= M_points) break; //reached M points limit
        }

        std::vector<std::pair<int, double>> results; //to store (index, distance) pairs
        results.reserve(candidates.size()); //reserving space

        for(auto index: candidates)
            results.emplace_back(static_cast<int>(index), vutils::euclideanDistance(query, stored_dataset[index])); //storing index and distance

        if(static_cast<int>(results.size()) > N)
            std::nth_element(results.begin(), results.begin() + N, results.end(), [](const auto& a,const auto& b){ return a.second < b.second; });
        
        std::sort(results.begin(), results.begin() + std::min(N, static_cast<int>(results.size())), [](const auto& a, const auto& b){ return a.second < b.second; });

        if(static_cast<int>(results.size()) > N)
            results.resize(N);
        
        return results;

    }

    std::vector<int>
    Hypercube::searchRadius(const std::vector<float>& query, double R) const {
        const std::string query_vertex = hashToVertex(query); // computing home vertex for query
        const std::vector<std::string> toVisit = enumerateProbes(query_vertex, std::max(1, probes_v)); //getting vertices to visit

        std::unordered_set<unsigned> candidates; //to avoid duplicates
        candidates.reserve(static_cast<int>(std::min(M_points, static_cast<int>(stored_dataset.size())))); //reserving space for candidates

        for(const auto& vertex: toVisit){
            auto it = cube_.find(vertex); //looking for vertex in cube
            if(it == cube_.end()) continue; //vertex not found, we continue
            for(auto index : it -> second){
                if(static_cast<int>(candidates.size()) >= M_points) break;
                candidates.insert(index); //adding index to candidates
            }

            if(static_cast<int>(candidates.size()) >= M_points) break; //reached M points limit
        }

        std::vector<int> inRange; //to store indices within radius R
        for(auto index: candidates){
            if(vutils::euclideanDistance(query, stored_dataset[index]) <=R)
                inRange.push_back(static_cast<int>(index)); //adding index to inRange
        }

        return inRange;
    }
}
