#include "vector_utils.h"

namespace vutils{

    //Global random generator
    static std::mt19937 randGen;
    static bool randGenInitialized = false;

    void initRand(unsigned seed){
        randGen.seed(seed);
        randGenInitialized = true;
    }

    double normalRand(){
        if(!randGenInitialized) initRand(1);
        static std::normal_distribution<double> distribution(0.0, 1.0);
        return dist(randGen);
        
    }


}