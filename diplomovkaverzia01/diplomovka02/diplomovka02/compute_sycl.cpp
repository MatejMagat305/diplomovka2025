#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"
#include  "fill_init_convert.h"

MemoryPointers GPUMemory{};
sycl::queue& getQueue() {
	static sycl::queue q{ sycl::default_selector_v };
	return q;
}

std::string initializeSYCLMemory(Map* m) {
	MemoryPointers& CPUMemory = m->CPUMemory;
	size_t mapSize = CPUMemory.width * CPUMemory.height;
	size_t stackSize = CPUMemory.agentsCount * mapSize;

	long long totalAlloc = 0;
	totalAlloc += sizeof(GPUMemory) * 2; // data structure
	totalAlloc += sizeof(int)*3; // minSize_numberColision_error
	totalAlloc += mapSize * sizeof(char); // grid
	totalAlloc += CPUMemory.agentsCount * sizeof(Agent); // agents
	totalAlloc += CPUMemory.loadersCount * sizeof(Position); // loaderPosition
	totalAlloc += CPUMemory.unloadersCount * sizeof(Position); // unloaderPosition
	totalAlloc += stackSize * sizeof(Position); // paths
	totalAlloc += CPUMemory.agentsCount * sizeof(int); // contraitsSizes
	totalAlloc += stackSize * sizeof(int); // gCost
	totalAlloc += stackSize * sizeof(int); // rhs
	totalAlloc += stackSize * sizeof(int); // fCost
	totalAlloc += stackSize * sizeof(Position); // cameFrom
	totalAlloc += stackSize * sizeof(bool); // visited
	totalAlloc += stackSize * sizeof(HeapPositionNode); // openList
	totalAlloc += stackSize * sizeof(Constraint) / 4; // constrait
	totalAlloc += CPUMemory.agentsCount * sizeof(int); // numberConstrait
	long long totalGlobalMem = 4LL * 1024 * 1024 * 1024; //q->get_device().get_info<sycl::info::device::global_mem_size>(); // 4 GB on cpu too
	if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
		std::stringstream ss;
		ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
		return ss.str();
	}
	try {
		sycl::queue& q = getQueue();
		GPUMemory.minSize_numberColision_error = sycl::malloc_device<int>(3, q);
		GPUMemory.grid = sycl::malloc_device<char>(mapSize, q);
		GPUMemory.agents = sycl::malloc_device<Agent>(CPUMemory.agentsCount, q);
		GPUMemory.loaderPositions = sycl::malloc_device<Position>(CPUMemory.loadersCount, q);
		GPUMemory.unloaderPositions = sycl::malloc_device<Position>(CPUMemory.unloadersCount, q);
		GPUMemory.agentPaths = sycl::malloc_device<Position>(stackSize, q);
		GPUMemory.gCost = sycl::malloc_device<int>(stackSize, q);
		GPUMemory.fCost = sycl::malloc_device<int>(stackSize, q);
		GPUMemory.cameFrom = sycl::malloc_device<Position>(stackSize, q);
		GPUMemory.visited = sycl::malloc_device<bool>(stackSize, q);
		GPUMemory.openList = sycl::malloc_device<HeapPositionNode>(stackSize, q);
		GPUMemory.constraints = sycl::malloc_device<Constraint>(stackSize / 4, q);
		GPUMemory.numberConstraints = sycl::malloc_device<int>(CPUMemory.agentsCount, q);
		sycl::event e1 = q.memcpy(GPUMemory.grid, CPUMemory.grid, mapSize * sizeof(char));
		sycl::event e2 = q.memcpy(GPUMemory.loaderPositions, CPUMemory.loaderPositions, CPUMemory.loadersCount * sizeof(Position));
		sycl::event e3 = q.memcpy(GPUMemory.unloaderPositions, CPUMemory.unloaderPositions, CPUMemory.unloadersCount * sizeof(Position));
		GPUMemory.width = CPUMemory.width;
		GPUMemory.height = CPUMemory.height;
		GPUMemory.loadersCount = CPUMemory.loadersCount;
		GPUMemory.unloadersCount = CPUMemory.unloadersCount;
		GPUMemory.agentsCount = CPUMemory.agentsCount;
		sycl::event::wait_and_throw({ e1, e2, e3 });
	}
	catch (sycl::exception& e) {
		std::stringstream ss;
		ss << e.what();
		return ss.str();
	}
	return "";
}

void synchronizeCPUFromGPU(Map* m) {
	MemoryPointers& CPUMemory = m->CPUMemory;
	sycl::queue& q = getQueue();
	sycl::event e1 = q.memcpy(CPUMemory.agents, GPUMemory.agents, CPUMemory.agentsCount * sizeof(Agent));
	sycl::event e3 = q.memcpy(CPUMemory.agentPaths, GPUMemory.agentPaths, CPUMemory.agentsCount * CPUMemory.width * CPUMemory.height * sizeof(Position));
	sycl::event e4 = q.memcpy(CPUMemory.minSize_numberColision_error, GPUMemory.minSize_numberColision_error, sizeof(int)*3);
	sycl::event::wait_and_throw({ e1, e3, e4 });
}

void synchronizeGPUFromCPU(Map* m) {
	MemoryPointers& CPUMemory = m->CPUMemory;
	sycl::queue& q = getQueue();
	sycl::event e1 = q.memcpy(GPUMemory.agents, CPUMemory.agents, CPUMemory.agentsCount * sizeof(Agent));
	sycl::event e3 = q.memcpy(GPUMemory.agentPaths, CPUMemory.agentPaths, CPUMemory.agentsCount * CPUMemory.width * CPUMemory.height * sizeof(Position));
	sycl::event e4 = q.memcpy(GPUMemory.minSize_numberColision_error, CPUMemory.minSize_numberColision_error, sizeof(int)*3);
	sycl::event::wait_and_throw({ e1, e3, e4 });
}

inline void freePtr(void** ptr) {
	if (ptr != nullptr && *ptr != nullptr) {
		sycl::free(*ptr, getQueue());
		*ptr = nullptr;
	}
}

void deleteGPUMem() {
	sycl::queue& q = getQueue();
	q.wait();
	freePtr((void**)(&(GPUMemory.minSize_numberColision_error)));
	freePtr((void**)(&(GPUMemory.unloaderPositions)));
	freePtr((void**)(&(GPUMemory.loaderPositions)));
	freePtr((void**)(&(GPUMemory.grid)));
	freePtr((void**)(&(GPUMemory.agents)));

	freePtr((void**)(&(GPUMemory.visited)));
	freePtr((void**)(&(GPUMemory.cameFrom)));
	freePtr((void**)(&(GPUMemory.fCost)));
	freePtr((void**)(&(GPUMemory.gCost)));
	freePtr((void**)(&(GPUMemory.openList)));
	freePtr((void**)(&(GPUMemory.agentPaths)));
	freePtr((void**)(&(GPUMemory.constraints)));
	freePtr((void**)(&(GPUMemory.numberConstraints)));
	q.wait();
}

void SYCL_AStar(Map* m) {
	sycl::queue& q = getQueue();
	int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
	// pohyb o minimálnu cestu 
	auto moveTO = q.submit([&](sycl::handler& h) {
		MemoryPointers globalMemory = GPUMemory;
		h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
			const int id = idx[0];
			MemoryPointers localMemory;
			fillLocalMemory(globalMemory, id, localMemory);
			moveAgentForIndex(id, localMemory);
			});
		});
	// cesty a kolízie
	auto collisionEvent = q.submit([&](sycl::handler& h) {
		h.depends_on(moveTO);
		MemoryPointers globalMemory = GPUMemory;
		sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)));
		h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
			const int id = item.get_global_id(0);
			MemoryPointers localMemory;
			fillLocalMemory(globalMemory, id, localMemory);
			astar_algorithm_get_sussces(id, localMemory);
			processAgentCollisionsGPU(globalMemory, localMemory, item);
			});
		});

	sycl::event minEvent = q.submit([&](sycl::handler& h) {
		h.depends_on(collisionEvent);
		MemoryPointers globalMemory = GPUMemory;
		h.single_task([=]() {
			writeMinimalPath(globalMemory);
			});
		});
	minEvent.wait_and_throw();
}

Info computeHYBRID(Map* m) {
	Info result{ 0,0 };
	try {
		auto start_time = std::chrono::high_resolution_clock::now();
		sycl::queue& q = getQueue();
		int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
		// pohyb o minimálnu cestu 
		auto moveTO = q.submit([&](sycl::handler& h) {
			MemoryPointers globalMemory = GPUMemory;
			h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
				const int id = idx[0];
				MemoryPointers localMemory;
				fillLocalMemory(globalMemory, id, localMemory);
				moveAgentForIndex(id, localMemory);
				});
			});
		// cesty
		auto foundPathsAgent = q.submit([&](sycl::handler& h) {
			h.depends_on(moveTO);
			MemoryPointers globalMemory = GPUMemory;
			h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
				const int id = idx[0];
				MemoryPointers localMemory;
				fillLocalMemory(globalMemory, id, localMemory);
				astar_algorithm_get_sussces(id, localMemory);
				localMemory.agents[id].sizePath = reconstructPath(id, localMemory);
				});
			});
		// kolízie
		synchronizeCPUFromGPU(m);
		CTNode root;
		root.paths.resize(GPUMemory.agentsCount);
		root.constraints.resize(GPUMemory.agentsCount);
		root.paths = getVector(m);
		auto solution = resolveConflictsCBS(m, root);
		auto end_time = std::chrono::high_resolution_clock::now();
		pushVector(solution, m);
		auto copy_time = std::chrono::high_resolution_clock::now();
		result.timeRun = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
		result.timeSynchronize = std::chrono::duration_cast<std::chrono::nanoseconds>(copy_time - end_time).count();
	}
	catch (const sycl::exception& e) {
		std::cerr << "SYCL exception caught: " << e.what() << std::endl;
		// Môžeš nastaviť nejaký error kód do result, alebo vrátiť default hodnoty
		result.timeRun = -1;
		result.timeSynchronize = -1;
	}
	catch (const std::exception& e) {
		std::cerr << "Standard exception caught: " << e.what() << std::endl;
		result.timeRun = -2;
		result.timeSynchronize = -2;
	}
	catch (...) {
		std::cerr << "Unknown exception caught!" << std::endl;
		result.timeRun = -3;
		result.timeSynchronize = -3;
	}
	return result;
}

Info computeSYCL(Map* m) {
	auto start_time = std::chrono::high_resolution_clock::now();
	SYCL_AStar(m);
	auto end_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
	return result;
}