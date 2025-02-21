#include "compute.h"
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"
#include "move_agent.h"
#include <chrono>
#include <thread>
#include <vector>


namespace internal_cpu {
    void parallelAStarPrimitive(Map& m) {
        std::vector<std::thread> threads;
        // Ka�d� agent dostane vlastn� vl�kno na v�po�et cesty
        for (int i = 0; i < m.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                internal::computePathForAgent(i, 
                    m.width_height_loaderCount_unloaderCount_agentCount, 
                    m.grid, 
                    m.agents,
                    m.loaderPosition, 
                    m.unloaderPosition, 
                    m.pathsAgent, 
                    m.pathSizesAgent, 
                    m.fCost, 
                    m.gCost, 
                    m.visited, 
                    m.cameFrom, 
                    m.openList);
                });
        }

        // Po�k�me, k�m v�etky vl�kna dokon�ia svoju pr�cu
        for (auto& t : threads) {
            t.join();
        }

        threads.clear(); // Vy�istenie vl�kien
        internal::MyBarrier b(m.agentCount);

        // Spracovanie kol�zi� v paraleln�ch vl�knach
        for (int i = 0; i < m.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                internal::processAgentCollisionsCPU(b, 
                    i, 
                    m.pathsAgent, 
                    m.pathSizesAgent, 
                    m.width_height_loaderCount_unloaderCount_agentCount,
                    m.minSize, 
                    m.grid, 
                    m.constrait, 
                    m.numberConstrait, 
                    m.agents);
                });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Pohyb agentov v paraleln�ch vl�knach
        for (int i = 0; i < m.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                internal::moveAgentForIndex(i, 
                    m.agents, 
                    m.pathsAgent, 
                    m.pathSizesAgent, 
                    m.loaderPosition, 
                    m.unloaderPosition,
                    m.width_height_loaderCount_unloaderCount_agentCount, 
                    m.minSize);
                });
        }
        for (auto& t : threads) {
            t.join();
        }
    }
}


double compute_cpu_primitive(AlgorithmType which, Map& m) {

    auto start_time = std::chrono::high_resolution_clock::now();
    switch (which) {
    case AlgorithmType::ASTAR:
        internal_cpu::parallelAStarPrimitive(m);
        break;
    default:
        internal_cpu::parallelAStarPrimitive(m);
        break;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end_time - start_time).count();
}

