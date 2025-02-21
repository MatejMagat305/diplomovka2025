#pragma once
#include "map.h"
#include "heap_primitive.h"

inline int getTrueIndexGrid(int width, int x, int y);
inline int myAbs(int input);

namespace internal {
    inline void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory);
    inline void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start);
}