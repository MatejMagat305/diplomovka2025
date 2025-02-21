#pragma once
#include "heap_primitive.h"
#include "map.h"

namespace internal {
    inline void computePathForAgent(int agentId, MemoryPointers& localMemory);
    inline void fillLocalMemory(const MemoryPointers& memory, int agentId, const MemoryPointers& local);
}

inline int getTrueIndexGrid(int width, int x, int y);
