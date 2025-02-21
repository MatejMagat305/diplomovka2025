#pragma once
#include "map.h"

enum class AlgorithmType {
    ASTAR,
    // DIJKSTRA TODO later
};
namespace internal {
    static bool wasGPUSynch = false;
}

double compute_cpu_primitive(AlgorithmType which, Map& m);
double computeSYCL(AlgorithmType which, Map& m);
double computeHybrid(AlgorithmType which, Map& m);

std::string initializeSYCL(Map& m);
void destroySYCL();
void synchronizeGPUFromCPU(Map& m);
void synchronizeCPUFromGPU(Map& m);

bool wasSynchronize() { return internal::wasGPUSynch; }
void setGPUSynch(bool b) { internal::wasGPUSynch = b; }