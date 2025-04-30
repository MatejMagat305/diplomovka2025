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
	m->print();
	auto paths = std::vector<std::vector<Position>>();
	paths.reserve(agentsCount);
    for (int agentID = 0; agentID < m->CPUMemory.agentsCount; agentID++){
		auto path = ComputeASTAR(m, agentID);
        if (path.empty()) {
            return Info{ 0,0, "CPU error computation in agent: " + std::to_string(agentID) };
        }
        paths.push_back(path);
    }
    CBSNode solution = solveCBS(paths);
    auto end_time = std::chrono::high_resolution_clock::now();
    if (solution.paths.empty()){
        return Info{ 0,0, "error in CBS " };
    }
    pushVector(solution.paths, m);
    auto copy_time = std::chrono::high_resolution_clock::now();
	writeMinimalPath(m->CPUMemory);

    Info result{ 0,0 };
    result.timeRun = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    result.timeSynchronize = std::chrono::duration_cast<std::chrono::nanoseconds>(copy_time - end_time).count();
    return result;
}