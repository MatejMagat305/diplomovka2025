#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include <chrono>
#include "fill_init_convert.h"
#include "a_star_algo.h"
#include "compute.h"

void computeInitialPaths(AlgorithmType which, Map& m, CTNode& root, int numThreads) {
	root.paths.resize(m.CPUMemory.agentCount);
	std::vector<std::thread> threads;
	std::mutex mtx;
	std::vector<bool> processedAgents(m.CPUMemory.agentCount, false);

	for (int i = 0; i < numThreads; i++) {
		threads.push_back(std::thread([&]() {
			while (true) {
				int agentId = 0;
				{
					std::unique_lock<std::mutex> lock(mtx);
					for (; agentId < m.CPUMemory.agentCount; agentId++) {
						if (!processedAgents[agentId]) {
							break;
						}
					}
					if (agentId == m.CPUMemory.agentCount) break;
					processedAgents[agentId] = true;
				}
				if (m.CPUMemory.agents[agentId].sizePath > 0){
					continue;
				}
				std::vector<std::vector<Constrait>> x(m.CPUMemory.agentCount, std::vector<Constrait>());
				std::vector<Position> path = ComputeCPUHIGHALGO(which, std::ref(m), agentId, x);
				root.paths[agentId] = path;
			}
			}));
	}
	for (auto& thread : threads) {
		thread.join();
	}

}

Info computeCPU(AlgorithmType which, Map& m, int numThreads) {
	CTNode root;
	root.constraints.resize(m.CPUMemory.agentCount);
	for (int i = 0; i < m.CPUMemory.agentCount; i++){
		moveAgentForIndex(i, m.CPUMemory);
	}
	auto start_time = std::chrono::high_resolution_clock::now();
	computeInitialPaths(which, m, root, numThreads);
	std::vector<std::vector<Position>> solution;
	resolveConflictsCBS(which, m, root, numThreads, solution);
	auto end_time = std::chrono::high_resolution_clock::now();
	pushVector(solution, m);
	auto copy_time = std::chrono::high_resolution_clock::now();
	Info result{};
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
	return result;
}