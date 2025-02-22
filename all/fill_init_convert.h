#pragma once
#include "map.h"
#include "heap_primitive.h"

inline int getTrueIndexGrid(int width, int x, int y);
inline int myAbs(int input);

namespace internal {
    inline void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory);
    inline void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start);


    std::vector<std::vector<Position>> getVector(Map& m);
    void pushVector(const std::vector<std::vector<Position>>& vec2D, Map& m);
    inline bool isSamePosition(const Position& a, const Position& b);
    int ManhattanHeuristic(const Position& a, const Position& b);
}