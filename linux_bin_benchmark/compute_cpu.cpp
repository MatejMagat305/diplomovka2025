#include "compute.h"
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include <chrono>
#include <thread>
#include <vector>
#include "fill_init_convert.h"
#include <queue>
#include <unordered_map>
#include <tuple>
#include <limits>

void parallelAStarPrimitive(Map* m) {
    std::vector<std::thread> threads;
    MemoryPointers& CPUMemory = m->CPUMemory;
    // pohyb o minimlnu cestu 
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        threads.emplace_back([&, i]() {
            MemoryPointers local;
            fillLocalMemory(CPUMemory, i, local);
            moveAgentForIndex(i, local);
        });
    }
    for (auto& t : threads) {
        t.join();
    }
	// std::cout << "move done" << std::endl;
    threads.clear();
    MyBarrier b(CPUMemory.agentsCount);

    // cesty a spracovanie kolzi v paralelnch vlknach
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        threads.emplace_back([&, i]() {
            MemoryPointers local;
            fillLocalMemory(CPUMemory, i, local);
            processAgentCollisionsCPU(CPUMemory, local, b, i);
        });
    }

    for (auto& t : threads) {
        t.join();
    }
	// std::cout << "collision done" << std::endl;
    writeMinimalPath(CPUMemory);
}

Info compute_cpu_primitive(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    parallelAStarPrimitive(m);    
    auto end_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return result;
}

Info compute_cpu_primitive_one_thread(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    processAgentCollisionsCPUOneThread(m->CPUMemory);
    auto end_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    return result;
}
