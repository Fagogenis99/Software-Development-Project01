#include "vector_utils.h"

namespace vutils{
    //Global random engine.We are using a Mersenne Twister engine and we'll use it 
    //everywhere we nedd random numbers ( for lsh, hypercube etc)
    static std::mt19937 rng;

    //keeping track if rng has already been seeded(to avoid reseeding it multiple times)
    static bool rngInitialized = false;

    //initializing rng with a given seed
    void initRand(unsigned seed){
        rng.seed(seed);
        rngInitialized = true;
    }

    //generating now a normally distributed random double: N(0,1)
    double normalRand(){
        if(!rngInitialized) initRand(1); //default seed = 1
        static std::normal_distribution<double> dist(0.0, 1.0);
        return dist(rng);
    }

    //generating a uniformly distributed random double in [a,b)
    //used for offsets t in lsh and hypercube
    double uniformRand(double a, double b){
        if(!rngInitialized) initRand(1);
        std::uniform_real_distribution<double> dist(a, b);
        return dist(rng);
    }

    //vector math functions...

    //computing the dot product between 2 vectors
     double dotProduct(const std::vector<float>& a, const std::vector<float>& b){
        size_t n = std::min(a.size(), b.size());
        return std::inner_product(a.begin(), a.begin() + n, b.begin(), 0.0);
     }

     double euclideanDistance(const std::vector<float>& a, const std::vector<float>& b){
        size_t n = std::min(a.size(), b.size());
        double sum = 0.0;
        for(size_t i = 0; i < n; ++i){
            double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
            sum += diff * diff;
        }
        return std::sqrt(sum);
     }
    
    // v = v/||v||
    void normalize(std::vector<float>& a){
        double norm =0.0;
        for (float x : a) norm += x * x;
        norm = std::sqrt(norm);
        if(norm == 0.0) return; //cannot normalize a zero vector
        for (float& x : a) x /= norm;
    }

    void printV(const std::vector<float>& a, const std::string& name){
        if(!name.empty()) std::cout << name << " = ";
        std::cout << "[ ";
        for (float x:a) std::cout << x << " ";
        std::cout << "]" << std::endl;
    }
    
}