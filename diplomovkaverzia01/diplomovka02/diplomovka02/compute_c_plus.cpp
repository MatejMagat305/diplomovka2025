#include <vector>
#include <queue>
#include <unordered_set>
#include <limits>
#include <chrono>
#include <algorithm>
#include "fill_init_convert.h"
#include "compute.h"
#include "a_star_algo.h"



Info computeCPU(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
	MemoryPointers& globalMemory = m->CPUMemory;
	int agentsCount = globalMemory.agentsCount;
    for (int agentID = 0; agentID < agentsCount; agentID++) {
        MemoryPointers localMemory;
        fillLocalMemory(globalMemory, agentID, localMemory);
        moveAgentForIndex(agentID, localMemory);
    }
    CTNode root;
    root.paths.resize(m->CPUMemory.agentsCount);
    root.constraints.resize(m->CPUMemory.agentsCount);
    for (int agentID = 0; agentID < m->CPUMemory.agentsCount; agentID++){
        root.paths[agentID] = ComputeASTAR(m, agentID, {});
    }
    std::vector<std::vector<Position>> solution = resolveConflictsCBS(m, root);
    auto end_time = std::chrono::high_resolution_clock::now();
    pushVector(solution, m);
    auto copy_time = std::chrono::high_resolution_clock::now();
	writeMinimalPath(m->CPUMemory);

    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
    result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
    return result;
}