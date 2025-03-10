#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "d_star_algo.h"
#include  "fill_init_convert.h"

MemoryPointers GPUMemory{};
sycl::queue& getQueue() {
	static sycl::queue q{ sycl::default_selector_v };
	return q;
}

std::string initializeSYCLMemory(Map& m) {
	size_t mapSize = m.CPUMemory.width * m.CPUMemory.height;
	size_t stackSize = m.CPUMemory.agentsCount * mapSize;

	long long totalAlloc = 0;
	totalAlloc += sizeof(GPUMemory) * 2; // data structuree
	totalAlloc += sizeof(int); // minSize
	totalAlloc += mapSize * sizeof(char); // grid
	totalAlloc += m.CPUMemory.agentsCount * sizeof(Agent); // agents
	totalAlloc += m.CPUMemory.loadersCount * sizeof(Position); // loaderPosition
	totalAlloc += m.CPUMemory.unloadersCount * sizeof(Position); // unloaderPosition
	totalAlloc += stackSize * sizeof(Position); // paths
	totalAlloc += m.CPUMemory.agentsCount * sizeof(int); // contraitsSizes
	totalAlloc += stackSize * sizeof(int); // gCost
	totalAlloc += stackSize * sizeof(int); // rhs
	totalAlloc += stackSize * sizeof(int); // fCost
	totalAlloc += stackSize * sizeof(Position); // cameFrom
	totalAlloc += stackSize * sizeof(bool); // visited
	totalAlloc += stackSize * sizeof(HeapPositionNode); // openList
	totalAlloc += stackSize * sizeof(Constrait) / 4; // constrait
	totalAlloc += m.CPUMemory.agentsCount * sizeof(int); // numberConstrait
	long long totalGlobalMem = 4LL * 1024 * 1024 * 1024; //q->get_device().get_info<sycl::info::device::global_mem_size>(); // 4 GB on cpu too
	if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
		std::stringstream ss;
		ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
		return ss.str();
	}
	try {
		sycl::queue& q = getQueue();
		GPUMemory.minSize_numberColision = sycl::malloc_device<int>(1, q);
		GPUMemory.grid = sycl::malloc_device<char>(mapSize, q);
		GPUMemory.agents = sycl::malloc_device<Agent>(m.CPUMemory.agentsCount, q);
		GPUMemory.loaderPositions = sycl::malloc_device<Position>(m.CPUMemory.loadersCount, q);
		GPUMemory.unloaderPositions = sycl::malloc_device<Position>(m.CPUMemory.unloadersCount, q);
		GPUMemory.agentPaths = sycl::malloc_device<Position>(stackSize, q);
		GPUMemory.gCost = sycl::malloc_device<int>(stackSize, q);
		GPUMemory.fCost_rhs = sycl::malloc_device<int>(stackSize, q);
		GPUMemory.cameFrom = sycl::malloc_device<Position>(stackSize, q);
		GPUMemory.visited = sycl::malloc_device<bool>(stackSize, q);
		GPUMemory.openList = sycl::malloc_device<HeapPositionNode>(stackSize, q);
		GPUMemory.constraits = sycl::malloc_device<Constrait>(stackSize / 4, q);
		GPUMemory.numberConstraits = sycl::malloc_device<int>(m.CPUMemory.agentsCount, q);
		sycl::event e1 = q.memcpy(GPUMemory.grid, m.CPUMemory.grid, mapSize * sizeof(char));
		sycl::event e2 = q.memcpy(GPUMemory.loaderPositions, m.CPUMemory.loaderPositions, m.CPUMemory.loadersCount * sizeof(Position));
		sycl::event e3 = q.memcpy(GPUMemory.unloaderPositions, m.CPUMemory.unloaderPositions, m.CPUMemory.unloadersCount * sizeof(Position));
		GPUMemory.width = m.CPUMemory.width;
		GPUMemory.height = m.CPUMemory.height;
		GPUMemory.loadersCount = m.CPUMemory.loadersCount;
		GPUMemory.unloadersCount = m.CPUMemory.unloadersCount;
		GPUMemory.agentsCount = m.CPUMemory.agentsCount;
		sycl::event::wait_and_throw({ e1, e2, e3 });
	}
	catch (sycl::exception& e) {
		std::stringstream ss;
		ss << e.what();
		return ss.str();
	}
	return "";
}

// Skopírovanie výsledkov do CPU
void synchronizeCPUFromGPU(Map& m) {
	sycl::queue& q = getQueue();
	sycl::event e1 = q.memcpy(m.CPUMemory.agents, GPUMemory.agents, m.CPUMemory.agentsCount * sizeof(Agent));
	sycl::event e3 = q.memcpy(m.CPUMemory.agentPaths, GPUMemory.agentPaths, m.CPUMemory.agentsCount * m.CPUMemory.width * m.CPUMemory.height * sizeof(Position));
	sycl::event e4 = q.memcpy(m.CPUMemory.minSize_numberColision, GPUMemory.minSize_numberColision, sizeof(int));
	sycl::event::wait_and_throw({ e1, e3, e4 });
}

// Skopírovanie výsledkov do GPU
void synchronizeGPUFromCPU(Map& m) {
	sycl::queue& q = getQueue();
	sycl::event e1 = q.memcpy(GPUMemory.agents, m.CPUMemory.agents, m.CPUMemory.agentsCount * sizeof(Agent));
	sycl::event e3 = q.memcpy(GPUMemory.agentPaths, m.CPUMemory.agentPaths, m.CPUMemory.agentsCount * m.CPUMemory.width * m.CPUMemory.height * sizeof(Position));
	sycl::event e4 = q.memcpy(GPUMemory.minSize_numberColision, m.CPUMemory.minSize_numberColision, sizeof(int));
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
	freePtr((void**)(&(GPUMemory.minSize_numberColision)));
	freePtr((void**)(&(GPUMemory.unloaderPositions)));
	freePtr((void**)(&(GPUMemory.loaderPositions)));
	freePtr((void**)(&(GPUMemory.grid)));
	freePtr((void**)(&(GPUMemory.agents)));

	freePtr((void**)(&(GPUMemory.visited)));
	freePtr((void**)(&(GPUMemory.cameFrom)));
	freePtr((void**)(&(GPUMemory.fCost_rhs)));
	freePtr((void**)(&(GPUMemory.gCost)));
	freePtr((void**)(&(GPUMemory.openList)));
	freePtr((void**)(&(GPUMemory.agentPaths)));
	freePtr((void**)(&(GPUMemory.constraits)));
	freePtr((void**)(&(GPUMemory.numberConstraits)));
	q.wait();
}

void SYCL_DStar(Map& m) {
	sycl::queue& q = getQueue();
	int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
	// pohyb o minimálnu cestu 
	auto moveTO = q.submit([&](sycl::handler& h) {
		MemoryPointers globalMemory = GPUMemory;
		h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), [=](sycl::id<1> idx) {
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
		sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)));
		h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
			const int id = item.get_global_id(0);
			MemoryPointers localMemory;
			fillLocalMemory(globalMemory, id, localMemory);
			setGCostFCostToINF(localMemory.width * localMemory.height, localMemory);
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


Info HYBRID_DStar_CBS(Map& m) {
	auto start_time = std::chrono::high_resolution_clock::now();
	sycl::queue& q = getQueue();
	int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
	// pohyb o minimálnu cestu 
	auto moveTO = q.submit([&](sycl::handler& h) {
		MemoryPointers globalMemory = GPUMemory;
		h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), [=](sycl::id<1> idx) {
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
		h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), [=](sycl::id<1> idx) {
			const int id = idx[0];
			MemoryPointers localMemory;
			fillLocalMemory(globalMemory, id, localMemory);
			positionHeap heap = { localMemory.openList, 0 };
			computePathForAgent_DStar(id, localMemory, heap);
			});
		});
	// kolízie
	synchronizeCPUFromGPU(m);
	CTNode root;
	root.constraints.resize(m.CPUMemory.agentsCount);
	root.paths = getVector(m);
	std::vector<std::vector<Position>> solution;
	resolveConflictsCBS(AlgorithmType::DSTAR, m, root, solution);
	auto end_time = std::chrono::high_resolution_clock::now();
	pushVector(solution, m);
	auto copy_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
	return result;
}

Info computeSYCL(AlgorithmType which, Map& m) {
	auto start_time = std::chrono::high_resolution_clock::now();
	switch (which) {
	case AlgorithmType::DSTAR:
		SYCL_DStar(m);
		break;
	default:
		SYCL_DStar(m);
		break;
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	return result;
}

Info computeHYBRID(AlgorithmType which, Map& m) {
	switch (which) {
	case AlgorithmType::DSTAR:
		return HYBRID_DStar_CBS(m);
	default:
		return HYBRID_DStar_CBS(m);
	}
}