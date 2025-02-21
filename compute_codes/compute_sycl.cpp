#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"
#include "move_agent.h"
#include <chrono>

namespace internal_gpu {
    MemoryPointers GPUMemory;
    sycl::queue& getQueue() {
        static sycl::queue q{ sycl::default_selector_v };
        return q;
    }

    std::string initializeSYCLMemory(Map& m) {
        size_t mapSize = m.width * m.height;
        size_t stackSize = m.agentCount * mapSize;

        long long totalAlloc = 0;
        totalAlloc += 6 * sizeof(int); // width_height_loaderCount_unloaderCount_agentCount + minSize
        totalAlloc += mapSize * sizeof(char); // grid
        totalAlloc += m.agentCount * sizeof(Agent); // agents
        totalAlloc += m.loaderCount * sizeof(Position); // loaderPosition
        totalAlloc += m.unloaderCount * sizeof(Position); // unloaderPosition
        totalAlloc += stackSize * sizeof(Position); // paths
        totalAlloc += m.agentCount * sizeof(int); // contraitsSizes
        totalAlloc += stackSize * sizeof(int); // gCost
        totalAlloc += stackSize * sizeof(int); // fCost
        totalAlloc += stackSize * sizeof(Position); // cameFrom
        totalAlloc += stackSize * sizeof(bool); // visited
        totalAlloc += stackSize * sizeof(Position); // openList
        totalAlloc += m.agentCount * (m.agentCount - 1) * sizeof(Constrait); // constrait
        totalAlloc += m.agentCount * sizeof(int); // numberConstrait
        long long totalGlobalMem = 4*1024*1024*1024; //q->get_device().get_info<sycl::info::device::global_mem_size>(); // 4 GB on cpu too
        if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
            std::stringstream ss;
            ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
            return ss.str();
        }
        try {
            sycl::queue& q = getQueue();
            GPUMemory.stuffWHLUA = sycl::malloc_device<int>(SIZE_INDEXES, q);
            GPUMemory.minSize = sycl::malloc_device<int>(1, q);
            GPUMemory.grid = sycl::malloc_device<char>(mapSize, q);
            GPUMemory.agents = sycl::malloc_device<Agent>(m.agentCount, q);
            GPUMemory.loaderPosition = sycl::malloc_device<Position>(m.loaderCount, q);
            GPUMemory.unloaderPosition = sycl::malloc_device<Position>(m.unloaderCount, q);
            GPUMemory.pathsAgent = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.gCost = sycl::malloc_device<int>(stackSize, q);
            GPUMemory.fCost = sycl::malloc_device<int>(stackSize, q);
            GPUMemory.cameFrom = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.visited = sycl::malloc_device<bool>(stackSize, q);
            GPUMemory.openList = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.constrait = sycl::malloc_device<Constrait>(m.agentCount * (m.agentCount - 1), q);
            GPUMemory.numberConstrait = sycl::malloc_device<int>(m.agentCount, q);
            int temp[SIZE_INDEXES] = { m.width, m.height, m.loaderCount, m.unloaderCount, m.agentCount};
            sycl::event e1 = q.memcpy(GPUMemory.grid, m.m.grid, mapSize * sizeof(char));
            sycl::event e2 = q.memcpy(GPUMemory.loaderPosition, m.m.loaderPosition, m.loaderCount * sizeof(Position));
            sycl::event e3 = q.memcpy(GPUMemory.unloaderPosition, m.m.unloaderPosition, m.unloaderCount * sizeof(Position));
            sycl::event e4 = q.memcpy(GPUMemory.stuffWHLUA, temp, SIZE_INDEXES * sizeof(int));
            sycl::event::wait({ e1, e2, e3, e4 });
        }
        catch (sycl::exception& e) {
            std::stringstream ss;
            auto& category = e.category();
            ss << e.what();
            return ss.str();
        }
        return "";
    }

    // Skopírovanie výsledkov do CPU
    void synchronizeCPUFromGPU(Map& m) {
        sycl::queue& q = getQueue();
        sycl::event e1 = q.memcpy(m.m.agents, GPUMemory.agents, m.agentCount * sizeof(Agent));
        sycl::event e3 = q.memcpy(m.m.pathsAgent, GPUMemory.pathsAgent, m.agentCount * m.width * m.height * sizeof(Position));
        sycl::event e4 = q.memcpy(m.m.minSize, GPUMemory.minSize, sizeof(int));
        sycl::event::wait_and_throw({ e1, e3, e4 });
    }

    // Skopírovanie výsledkov do GPU
    void synchronizeGPUFromCPU(Map& m){
        sycl::queue& q = getQueue();
        sycl::event e1 = q.memcpy(GPUMemory.agents, m.m.agents, m.agentCount * sizeof(Agent));
        sycl::event e3 = q.memcpy(GPUMemory.pathsAgent, m.m.pathsAgent, m.agentCount * m.width * m.height * sizeof(Position));
        sycl::event e4 = q.memcpy(GPUMemory.minSize, m.m.minSize, sizeof(int));
        sycl::event::wait_and_throw({e1, e3, e4});
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
        freePtr((void**)(&(GPUMemory.stuffWHLUA)));
        freePtr((void**)(&(GPUMemory.minSize)));
        freePtr((void**)(&(GPUMemory.unloaderPosition)));
        freePtr((void**)(&(GPUMemory.loaderPosition)));
        freePtr((void**)(&(GPUMemory.grid)));
        freePtr((void**)(&(GPUMemory.agents)));

        freePtr((void**)(&(GPUMemory.visited)));
        freePtr((void**)(&(GPUMemory.cameFrom)));
        freePtr((void**)(&(GPUMemory.fCost)));
        freePtr((void**)(&(GPUMemory.gCost)));
        freePtr((void**)(&(GPUMemory.openList)));
        freePtr((void**)(&(GPUMemory.pathsAgent)));
        freePtr((void**)(&(GPUMemory.constrait)));
        freePtr((void**)(&(GPUMemory.numberConstrait)));
        q.wait();
    }

    void parallelAStar(Map& m) {
        sycl::queue& q = getQueue();
        int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
        // pohyb o minimálnu cestu 
        auto moveTO = q.submit([&](sycl::handler& h) {

            h.parallel_for(sycl::range<1>(std::min(workSize, m.agentCount)), [=](sycl::id<1> idx) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
                const int id = idx[0];
                if (id > agentsize - 1 || *minSize < 1) {
                    return;
                }
                MemoryPointers global = {}
                internal::moveAgentForIndex(id, agents, pathsAgent,
                    loaderPosition, unloaderPosition,
                    width_height_loaderCount_unloaderCount_agentCount, minSize);
                });
            });
        // cesty
        auto foundPathsAgent = q.submit([&](sycl::handler& h) {
            h.depends_on(moveTO);
            h.parallel_for(sycl::range<1>(std::min(workSize, m.agentCount)), [=](sycl::id<1> idx) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
                const int id = idx[0];
                if (id == 0)   {
                    *minSize = 2147483647; // nastav minSize pre daľší kernel
                }
                if (id>agentsize-1)      {
                    return;
                }
                internal::computePathForAgent(id, width_height_loaderCount_unloaderCount_agentCount, grid, agents,
                    loaderPosition, unloaderPosition, pathsAgent, fCost, gCost, visited, cameFrom, openList);
                });
            });
        // kolízie
        auto collisionEvent = q.submit([&](sycl::handler& h) {
            h.depends_on(foundPathsAgent);
            sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, m.agentCount)), sycl::range<1>(std::min(workSize, m.agentCount)));
            h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
                if (item.get_global_id(0) > agentsize - 1) {
                    return;
                }
                internal::processAgentCollisionsGPU(item, pathsAgent, 
                    width_height_loaderCount_unloaderCount_agentCount, minSize,
                    grid, constrait, numberConstrait, agents);
                });
            });

        collisionEvent.wait_and_throw();
    }
}


double computeSYCL(AlgorithmType which, Map& m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    switch (which)    {
    case AlgorithmType::ASTAR:
        internal_gpu::parallelAStar(m);
        break;
    default:
        break;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end_time - start_time).count();
}


std::string initializeSYCL(Map& m) {
    std::string res = internal_gpu::initializeSYCLMemory(m);
    if (res != "") {
        destroySYCL();
    }
    return res;
}

void destroySYCL() {
    internal_gpu::deleteGPUMem();
}

void synchronizeGPUFromCPU(Map& m) {
    internal_gpu::synchronizeGPUFromCPU(m);
}


void synchronizeCPUFromGPU(Map& m) {
    internal_gpu::synchronizeCPUFromGPU(m);
}
