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
#include "fill_init_convert.h"


namespace internal_cpu {
    void parallelAStarPrimitive(Map& m) {
        std::vector<std::thread> threads;
        // Každý agent dostane vlastné vlákno na výpoèet cesty
        for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                MemoryPointers local;
                internal::fillLocalMemory(m.CPUMemory, i, local);
                internal::computePathForAgent(i, local);
                });
        }

        // Poèkáme, kým všetky vlákna dokonèia svoju prácu
        for (auto& t : threads) {
            t.join();
        }

        threads.clear(); // Vyèistenie vlákien
        internal::MyBarrier b(m.CPUMemory.agentCount);

        // Spracovanie kolízií v paralelných vláknach
        for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                MemoryPointers local;
                internal::fillLocalMemory(m.CPUMemory, i, local);
                internal::processAgentCollisionsCPU(b, i, m.CPUMemory, local);
                });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Pohyb agentov v paralelných vláknach
        for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
            threads.emplace_back([&, i]() {
                MemoryPointers local;
                internal::fillLocalMemory(m.CPUMemory, i, local);
                internal::moveAgentForIndex(i, local);
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

