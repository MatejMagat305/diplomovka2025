#pragma once
#include "map.h"
#include "device_type_algoritmus.h"
#include "sycl/sycl.hpp"

Info compute_cpu_primitive(AlgorithmType which, Map& m);
Info computeSYCL(AlgorithmType which, Map& m);
Info computeHYBRID(AlgorithmType which, Map& m);
Info computeCPU(AlgorithmType which, Map& m, int numThreads);
std::string initializeSYCLMemory(Map& m);

SYCL_EXTERNAL void moveAgentForIndex(int id, MemoryPointers& localMemory);
void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, int numThreads, std::vector<std::vector<Position>>& solution);

void synchronizeGPUFromCPU(Map& m);
void synchronizeCPUFromGPU(Map& m);