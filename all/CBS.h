#pragma once
#include "map.h"
#include "compute.h"

namespace internal {
    void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, int numThreads, std::vector<std::vector<Position>>& solution);
}