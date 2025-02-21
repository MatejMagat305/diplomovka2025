#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"
#include  "fill_init_convert.h"
#include "move_agent.h"
#include <chrono>

namespace internal_gpu {
    MemoryPointers GPUMemory;
    sycl::queue& getQueue() {
        static sycl::queue q{ sycl::default_selector_v };
        return q;
    }

    std::string initializeSYCLMemory(Map& m) {
        size_t mapSize = m.CPUMemory.width * m.CPUMemory.height;
        size_t stackSize = m.CPUMemory.agentCount * mapSize;

        long long totalAlloc = 0;
        totalAlloc += sizeof(GPUMemory)*2; // data structuree
        totalAlloc += sizeof(int); // minSize
        totalAlloc += mapSize * sizeof(char); // grid
        totalAlloc += m.CPUMemory.agentCount * sizeof(Agent); // agents
        totalAlloc += m.CPUMemory.loaderCount * sizeof(Position); // loaderPosition
        totalAlloc += m.CPUMemory.unloaderCount * sizeof(Position); // unloaderPosition
        totalAlloc += stackSize * sizeof(Position); // paths
        totalAlloc += m.CPUMemory.agentCount * sizeof(int); // contraitsSizes
        totalAlloc += stackSize * sizeof(int); // gCost
        totalAlloc += stackSize * sizeof(int); // fCost
        totalAlloc += stackSize * sizeof(Position); // cameFrom
        totalAlloc += stackSize * sizeof(bool); // visited
        totalAlloc += stackSize * sizeof(Position); // openList
        totalAlloc += stackSize * sizeof(Constrait) / 4; // constrait
        totalAlloc += m.CPUMemory.agentCount * sizeof(int); // numberConstrait
        long long totalGlobalMem = 4*1024*1024*1024; //q->get_device().get_info<sycl::info::device::global_mem_size>(); // 4 GB on cpu too
        if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
            std::stringstream ss;
            ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
            return ss.str();
        }
        try {
            sycl::queue& q = getQueue();
            GPUMemory.minSize = sycl::malloc_device<int>(1, q);
            GPUMemory.grid = sycl::malloc_device<char>(mapSize, q);
            GPUMemory.agents = sycl::malloc_device<Agent>(m.CPUMemory.agentCount, q);
            GPUMemory.loaderPosition = sycl::malloc_device<Position>(m.CPUMemory.loaderCount, q);
            GPUMemory.unloaderPosition = sycl::malloc_device<Position>(m.CPUMemory.unloaderCount, q);
            GPUMemory.pathsAgent = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.gCost = sycl::malloc_device<int>(stackSize, q);
            GPUMemory.fCost = sycl::malloc_device<int>(stackSize, q);
            GPUMemory.cameFrom = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.visited = sycl::malloc_device<bool>(stackSize, q);
            GPUMemory.openList = sycl::malloc_device<Position>(stackSize, q);
            GPUMemory.constrait = sycl::malloc_device<Constrait>(stackSize/4, q);
            GPUMemory.numberConstrait = sycl::malloc_device<int>(m.CPUMemory.agentCount, q);
            sycl::event e1 = q.memcpy(GPUMemory.grid, m.CPUMemory.grid, mapSize * sizeof(char));
            sycl::event e2 = q.memcpy(GPUMemory.loaderPosition, m.CPUMemory.loaderPosition, m.CPUMemory.loaderCount * sizeof(Position));
            sycl::event e3 = q.memcpy(GPUMemory.unloaderPosition, m.CPUMemory.unloaderPosition, m.CPUMemory.unloaderCount * sizeof(Position));
            GPUMemory.width = m.CPUMemory.width;
            GPUMemory.height = m.CPUMemory.height;
            GPUMemory.loaderCount = m.CPUMemory.loaderCount;
            GPUMemory.unloaderCount = m.CPUMemory.unloaderCount;
            GPUMemory.agentCount = m.CPUMemory.agentCount;
            sycl::event::wait_and_throw({ e1, e2, e3 });
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
        sycl::event e1 = q.memcpy(m.CPUMemory.agents, GPUMemory.agents, m.CPUMemory.agentCount * sizeof(Agent));
        sycl::event e3 = q.memcpy(m.CPUMemory.pathsAgent, GPUMemory.pathsAgent, m.CPUMemory.agentCount * m.CPUMemory.width * m.CPUMemory.height * sizeof(Position));
        sycl::event e4 = q.memcpy(m.CPUMemory.minSize, GPUMemory.minSize, sizeof(int));
        sycl::event::wait_and_throw({ e1, e3, e4 });
    }

    // Skopírovanie výsledkov do GPU
    void synchronizeGPUFromCPU(Map& m){
        sycl::queue& q = getQueue();
        sycl::event e1 = q.memcpy(GPUMemory.agents, m.CPUMemory.agents, m.CPUMemory.agentCount * sizeof(Agent));
        sycl::event e3 = q.memcpy(GPUMemory.pathsAgent, m.CPUMemory.pathsAgent, m.CPUMemory.agentCount * m.CPUMemory.width * m.CPUMemory.height * sizeof(Position));
        sycl::event e4 = q.memcpy(GPUMemory.minSize, m.CPUMemory.minSize, sizeof(int));
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
            MemoryPointers globalMemory = GPUMemory;
            h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentCount)), [=](sycl::id<1> idx) {
                const int id = idx[0];
                if (id > globalMemory.agentCount - 1 || *globalMemory.minSize < 1) {
                    return;
                }
                MemoryPointers localMemory;
                internal::fillLocalMemory(globalMemory, id, localMemory);
                internal::moveAgentForIndex(id, localMemory);
                });
            });
        // cesty
        auto foundPathsAgent = q.submit([&](sycl::handler& h) {
            h.depends_on(moveTO);
            MemoryPointers globalMemory = GPUMemory;
            h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentCount)), [=](sycl::id<1> idx) {
                const int id = idx[0];
                if (id == 0)   {
                    *globalMemory.minSize = 2147483647; // nastav minSize pre daľší kernel
                }
                if (id>globalMemory.agentCount - 1){
                    return;
                }
                MemoryPointers localMemory;
                internal::fillLocalMemory(globalMemory, id, localMemory);
                internal::computePathForAgent(id, localMemory);
                });
            });
        // kolízie
        auto collisionEvent = q.submit([&](sycl::handler& h) {
            h.depends_on(foundPathsAgent);
            MemoryPointers globalMemory = GPUMemory;
            sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, m.CPUMemory.agentCount)), sycl::range<1>(std::min(workSize, m.CPUMemory.agentCount)));
            h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                const int id = item.get_global_id(0);
                if (id > globalMemory.agentCount - 1) {
                    return;
                }
                MemoryPointers localMemory;
                internal::fillLocalMemory(globalMemory, id, localMemory);
                internal::processAgentCollisionsGPU(item, localMemory);
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
