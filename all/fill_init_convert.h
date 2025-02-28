#pragma once
#include "map.h"
#include "heap_primitive.h"
#include "sycl/sycl.hpp"

SYCL_EXTERNAL int getTrueIndexGrid(int width, int x, int y);
SYCL_EXTERNAL int myAbs(int input);

SYCL_EXTERNAL void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory);
SYCL_EXTERNAL void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start);


std::vector<std::vector<Position>> getVector(Map& m);
void pushVector(const std::vector<std::vector<Position>>& vec2D, Map& m);
bool isSamePosition(const Position& a, const Position& b);
int ManhattanHeuristic(const Position& a, const Position& b);