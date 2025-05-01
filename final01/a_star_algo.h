#pragma once
#include "device_type_algoritmus.h"
#include "map.h"
#include "sycl/sycl.hpp"

SYCL_EXTERNAL bool astar_algorithm_get_sussces(int agentId, MemoryPointers& localMemory);
SYCL_EXTERNAL void fillLocalMemory(const MemoryPointers& memory, int agentId, const MemoryPointers& local);
    
std::vector<Position> ComputeASTAR(Map* m, int agentID);

