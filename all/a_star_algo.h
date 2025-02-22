#pragma once
#include "heap_primitive.h"
#include "map.h"

namespace internal {
    inline void computePathForAgent(int agentId, MemoryPointers& localMemory);
    inline void fillLocalMemory(const MemoryPointers& memory, int agentId, const MemoryPointers& local);

    std::vector<Position> ComputeCPUHIGHALGO(AlgorithmType which, Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints);
}

inline int getTrueIndexGrid(int width, int x, int y);
