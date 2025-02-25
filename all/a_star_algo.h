#pragma once
#include "device_type_algoritmus.h"
#include "map.h"
#include "sycl/sycl.hpp"

SYCL_EXTERNAL void computePathForAgent(int agentId, MemoryPointers& localMemory);
SYCL_EXTERNAL void fillLocalMemory(const MemoryPointers& memory, int agentId, const MemoryPointers& local);
    
std::vector<Position> ComputeCPUHIGHALGO(AlgorithmType which, Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints);

