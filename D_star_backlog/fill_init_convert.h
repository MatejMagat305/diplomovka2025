#pragma once
#include "map.h"
#include "heap_primitive.h"
#include "sycl/sycl.hpp"

#define INF 1000000

SYCL_EXTERNAL int getTrueIndexGrid(int width, int x, int y);

SYCL_EXTERNAL int myAbs(int input);

SYCL_EXTERNAL void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory);

SYCL_EXTERNAL void initializeCostsAndHeap(MemoryPointers& localMemory, positionHeap& myHeap, Position start);

SYCL_EXTERNAL int reconstructPath(int agentID, MemoryPointers& localMemory, Position start, Position goal);

SYCL_EXTERNAL void writeMinimalPath(const MemoryPointers& globalMemory);

SYCL_EXTERNAL void setGCostFCostToINF(int mapSize, MemoryPointers& localMemory);

SYCL_EXTERNAL int ManhattanHeuristic(const Position& a, const Position& b);

SYCL_EXTERNAL void moveAgentForIndex(int id, MemoryPointers& localMemory);

std::vector<std::vector<Position>> getVector(Map& m);

void pushVector(const std::vector<std::vector<Position>>& vec2D, Map& m);
