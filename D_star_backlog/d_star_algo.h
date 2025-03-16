#pragma once
#include "map.h"
#include "sycl/sycl.hpp"


struct Key {
    int first;
    int second;
};

SYCL_EXTERNAL Key computeKey(MemoryPointers& localMemory, Position s, Position start);
SYCL_EXTERNAL void updateVertex_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position s, Position start);
SYCL_EXTERNAL void computeShortestPath_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position start, Position goal);
SYCL_EXTERNAL void computePathForAgent_DStar(int agentId, MemoryPointers& localMemory, positionHeap& heap);
SYCL_EXTERNAL bool isSamePosition(Position &a, Position &b);
SYCL_EXTERNAL void setGCostFCostToINF(int mapSize, MemoryPointers& localMemory);
