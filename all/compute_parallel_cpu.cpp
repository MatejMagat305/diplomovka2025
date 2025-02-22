#include "compute.h"
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
#include "CBS.h"

namespace internal {

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

                    std::vector<Position> path = ComputeCPUHIGHALGO(which, std::ref(m), agentId, const std::vector<std::vector<Constrait>>{});
                    root.paths[agentId] = path;
                }
                }));
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    void resolveConflictsCBS_CPU(AlgorithmType which, Map& m, int numThreads) {
        CTNode root;
        computeInitialPaths(which, m, root, numThreads);
        std::vector<std::vector<Position>> solution;
        resolveConflictsCBS(which, m, root, numThreads, solution);
        pushVector(solution, m);
    }
}

double computeCPU(AlgorithmType which, Map& m, int numThreads) {
    auto start_time = std::chrono::high_resolution_clock::now();
    internal::resolveConflictsCBS_CPU(which, m, numThreads);
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end_time - start_time).count();
}

