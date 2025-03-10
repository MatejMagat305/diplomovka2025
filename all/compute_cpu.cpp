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


void parallelDStarPrimitive(Map& m) {
	std::vector<std::thread> threads;
	// pohyb o minimálnu cestu 
	for (int i = 0; i < m.CPUMemory.agentsCount; ++i) {
		threads.emplace_back([&, i]() {
			MemoryPointers local;
			fillLocalMemory(m.CPUMemory, i, local);
			moveAgentForIndex(i, local);
			});
	}
	for (auto& t : threads) {
		t.join();
	}

	threads.clear();
	MyBarrier b(m.CPUMemory.agentsCount);

	// cesty a spracovanie kolízií v paralelných vláknach
	for (int i = 0; i < m.CPUMemory.agentsCount; ++i) {
		threads.emplace_back([&, i]() {
			MemoryPointers local;
			fillLocalMemory(m.CPUMemory, i, local);
			processAgentCollisionsCPU(m.CPUMemory, local, b, i);
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
	case AlgorithmType::DSTAR:
		parallelDStarPrimitive(m);
		break;
	default:
		parallelDStarPrimitive(m);
		break;
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	return result;
}

void DStarPrimitiveOneThread(Map& m) {
	processAgentCollisionsCPUOneThread(m.CPUMemory);
}

Info compute_cpu_primitive_one_thread(AlgorithmType which, Map& m) {
	auto start_time = std::chrono::high_resolution_clock::now();
	switch (which) {
	case AlgorithmType::DSTAR:
		DStarPrimitiveOneThread(m);
		break;
	default:
		DStarPrimitiveOneThread(m);
		break;
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	return result;
}

