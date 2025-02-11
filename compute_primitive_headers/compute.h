#pragma once
#include "map.h"

enum class AlgorithmType {
    ASTAR,
    // DIJKSTRA TODO later
};
namespace internal {
    static bool wasGPUSynch = false;
    bool wasSynchronize() { return wasGPUSynch; }
    void setGPUSynch(bool b) { wasGPUSynch = b; }
}

double computeCPU(AlgorithmType which, Map& m, int numThreads);
double computeSYCL(AlgorithmType which, Map& m);

void deleteCPUStruct();
