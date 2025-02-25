#include "compute.h"
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"
#include <chrono>
#include <thread>
#include <vector>
#include "fill_init_convert.h"

void parallelAStarPrimitive(Map& m) {
	std::vector<std::thread> threads;
	// pohyb o minimálnu cestu 
	for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
		threads.emplace_back([&, i]() {
			MemoryPointers local;
			fillLocalMemory(m.CPUMemory, i, local);
			moveAgentForIndex(i, local);
			});
	}
	for (auto& t : threads) {
		t.join();
	}
	// cesty
	threads.clear();
	for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
		threads.emplace_back([&, i]() {
			MemoryPointers local;
			fillLocalMemory(m.CPUMemory, i, local);
			computePathForAgent(i, local);
			});
	}
	for (auto& t : threads) {
		t.join();
	}

	threads.clear();
	MyBarrier b(m.CPUMemory.agentCount);

	// Spracovanie kolízií v paralelných vláknach
	for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
		threads.emplace_back([&, i]() {
			MemoryPointers local;
			fillLocalMemory(m.CPUMemory, i, local);
			processAgentCollisionsCPU(b, i, m.CPUMemory, local);
			});
	}

	for (auto& t : threads) {
		t.join();
	}
	writeMinimalPath(m.CPUMemory);
}

Info compute_cpu_primitive(AlgorithmType which, Map& m) {
	auto start_time = std::chrono::high_resolution_clock::now();
	switch (which) {
	case AlgorithmType::ASTAR:
		parallelAStarPrimitive(m);
		break;
	default:
		parallelAStarPrimitive(m);
		break;
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	return result;
}
