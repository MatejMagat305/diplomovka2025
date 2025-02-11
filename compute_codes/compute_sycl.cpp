#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include "heap_primitive.h"
#include "solve_conflicts.h"
#include "a_star_algo.h"

namespace internal {
int* width_height_loaderCount_unloaderCount_agentCount_minSize;
char* grid = nullptr;
Position* loaderPosition = nullptr;
Position* unloaderPosition = nullptr;
Agent* agents = nullptr;
Position* paths = nullptr;
int* pathSizes = nullptr;

int* gCost = nullptr;
int* fCost = nullptr;
Position* cameFrom = nullptr;
bool* visited = nullptr;
Position* openList = nullptr;
Constrait* constrait = nullptr;
int* numberConstrait = nullptr;
sycl::queue* q = nullptr;


    std::string initializeSYCLMemory(Map& m) {
        size_t mapSize = m.width * m.height;
        size_t stackSize = m.agentCount * mapSize;

        size_t totalAlloc = 0;
        totalAlloc += 6 * sizeof(int); // width_height_loaderCount_unloaderCount_agentCount
        totalAlloc += mapSize * sizeof(char); // grid
        totalAlloc += m.agentCount * sizeof(Agent); // agents
        totalAlloc += m.loaderCount * sizeof(Position); // loaderPosition
        totalAlloc += m.unloaderCount * sizeof(Position); // unloaderPosition
        totalAlloc += stackSize * sizeof(Position); // paths
        totalAlloc += m.agentCount * sizeof(int); // pathSizes
        totalAlloc += m.agentCount * sizeof(int); // contraitsSizes
        totalAlloc += stackSize * sizeof(int); // gCost
        totalAlloc += stackSize * sizeof(int); // fCost
        totalAlloc += stackSize * sizeof(Position); // cameFrom
        totalAlloc += stackSize * sizeof(bool); // visited
        totalAlloc += stackSize * sizeof(Position); // openList
        totalAlloc += m.agentCount * (m.agentCount - 1) * sizeof(Constrait); // constrait
        totalAlloc += m.agentCount * sizeof(int); // numberConstrait
        size_t totalGlobalMem = q->get_device().get_info<sycl::info::device::global_mem_size>();
        if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
            std::stringstream ss;
            ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
            return ss.str();
        }
        try {
            width_height_loaderCount_unloaderCount_agentCount_minSize = sycl::malloc_device<int>(SIZE_INDEXES, (*q));
            grid = sycl::malloc_device<char>(mapSize, (*q));
            agents = sycl::malloc_device<Agent>(m.agentCount, (*q));
            loaderPosition = sycl::malloc_device<Position>(m.loaderCount, (*q));
            unloaderPosition = sycl::malloc_device<Position>(m.unloaderCount, (*q));
            paths = sycl::malloc_device<Position>(stackSize, (*q));
            pathSizes = sycl::malloc_device<int>(m.agentCount, (*q));
            gCost = sycl::malloc_device<int>(stackSize, (*q));
            fCost = sycl::malloc_device<int>(stackSize, (*q));
            cameFrom = sycl::malloc_device<Position>(stackSize, (*q));
            visited = sycl::malloc_device<bool>(stackSize, (*q));
            openList = sycl::malloc_device<Position>(stackSize, (*q));
            constrait = sycl::malloc_device<Constrait>(m.agentCount * (m.agentCount - 1), (*q));
            numberConstrait = sycl::malloc_device<int>(m.agentCount, (*q));
        }
        catch (sycl::exception& e) {
            std::stringstream ss;
            auto& category = e.category();
            ss << e.what();
            return ss.str();
        }
        return "";
    }

    void synchGPU(Map& m)    {
        if (wasSynchronize())        {
            return;
        }
        setGPUSynch(true);
        int mapSize = m.width * m.height;
        int temp[SIZE_INDEXES] = { m.width, m.height, m.loaderCount, m.unloaderCount, m.agentCount, std::numeric_limits<int>::max() };
        sycl::event e1 = q->memcpy(grid, m.grid, mapSize * sizeof(char));
        sycl::event e2 = q->memcpy(loaderPosition, m.loaderPosition, m.loaderCount * sizeof(Position));
        sycl::event e3 = q->memcpy(unloaderPosition, m.unloaderPosition, m.unloaderCount * sizeof(Position));
        sycl::event e4 = q->memcpy(agents, m.agents, m.agentCount * sizeof(Agent));
        sycl::event e5 = q->memcpy(width_height_loaderCount_unloaderCount_agentCount_minSize, temp, SIZE_INDEXES * sizeof(int));
        sycl::event e6 = (*q).memset(pathSizes, 0, m.agentCount * sizeof(int));
        sycl::event::wait_and_throw({ e1, e2, e3, e4, e5, e6 });

    }

    inline void freePtr(void** ptr) {
        if (ptr != nullptr && *ptr != nullptr) {
            sycl::free(*ptr, *q);
            *ptr = nullptr;
        }
    }

    void deleteGPUMem() {
        if (q) {
            q->wait();
            freePtr((void**)(& width_height_loaderCount_unloaderCount_agentCount_minSize));
            freePtr((void**)(&unloaderPosition));
            freePtr((void**)(&loaderPosition));
            freePtr((void**)(&grid));
            freePtr((void**)(&agents));

            freePtr((void**)(&visited));
            freePtr((void**)(&cameFrom));
            freePtr((void**)(&fCost));
            freePtr((void**)(&gCost));
            freePtr((void**)(&openList));
            freePtr((void**)(&paths));
            freePtr((void**)(&pathSizes));
            freePtr((void**)(&constrait));
            freePtr((void**)(&numberConstrait));
            q->wait();
            delete q;
            q = nullptr;
        }
    }

    void moveAgentForIndex(int id, Agent* agents, Position* paths, int* pathSizes, Position* loaderPosition, Position* unloaderPosition,
        int* width_height_loaderCount_unloaderCount_agentCount_minSize) {
        Agent& a = agents[id];
        const int moveSize = width_height_loaderCount_unloaderCount_agentCount_minSize[MIN_INDEX];
        const int width = width_height_loaderCount_unloaderCount_agentCount_minSize[WIDTHS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount_minSize[HEIGHTS_INDEX];
        int offset = id * width * height;
        if (moveSize > 0) {
            Position& p = paths[offset + moveSize - 1];
            a.x = p.x;
            a.y = p.y;
        }
        else {
            return;
        }

        if (a.direction == AGENT_LOADER) {
            Position& loader = loaderPosition[a.loaderCurrent];
            if (a.x == loader.x && a.y == loader.y) {
                a.direction = AGENT_UNLOADER;
                a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % width_height_loaderCount_unloaderCount_agentCount_minSize[LOADERS_INDEX];
                // TODO random
                pathSizes[id] = 0;
                return;
            }
        }
        else {
            Position& unloader = unloaderPosition[a.unloaderCurrent];
            if (a.x == unloader.x && a.y == unloader.y) {
                a.direction = AGENT_LOADER;
                a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % width_height_loaderCount_unloaderCount_agentCount_minSize[UNLOADERS_INDEX];
                pathSizes[id] = 0;
                return;
            }
        }
        Position* agentPath = &paths[offset];
        int agentSizePath = pathSizes[id];
        for (int i = moveSize; i < agentSizePath; i++){
            agentPath[i - moveSize] = agentPath[i];
        }
        pathSizes[id] = pathSizes[id] - moveSize;
    }

    void parallelAStar(Map& m) {
        int workSize = q->get_device().get_info<sycl::info::device::max_work_group_size>();
        // cesty
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(std::min(workSize, m.agentCount)), [=](sycl::id<1> idx) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount_minSize[AGENTS_INDEX];
                if (idx[0]>agentsize-1)      {
                    return;
                }
                computePathForAgent(idx[0], width_height_loaderCount_unloaderCount_agentCount_minSize, grid, agents,
                    loaderPosition, unloaderPosition, paths, pathSizes, fCost, gCost, visited, cameFrom, openList);
                });
            });
        // kolízie
        auto collisionEvent = (*q).submit([&](sycl::handler& h) {
            h.depends_on(e);
            sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, m.agentCount)), sycl::range<1>(std::min(workSize, m.agentCount)));
            h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount_minSize[AGENTS_INDEX];
                if (item.get_global_id(0) > agentsize - 1) {
                    return;
                }
                processAgentCollisions(item, paths, 
                    pathSizes, width_height_loaderCount_unloaderCount_agentCount_minSize,
                    grid, constrait, numberConstrait, agents);
                });
            });

        collisionEvent.wait();
        // Skopírovanie výsledkov do CPU
        q->memcpy(m.agentPaths, paths, m.agentCount * m.width * m.height * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
        // pohyb o minimálnu cestu 
        auto moveTO = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(std::min(workSize, m.agentCount)), [=](sycl::id<1> idx) {
                const int agentsize = width_height_loaderCount_unloaderCount_agentCount_minSize[AGENTS_INDEX];
                if (idx[0] > agentsize - 1) {
                    return;
                }
                moveAgentForIndex(idx[0], agents, paths, pathSizes,
                    loaderPosition, unloaderPosition, 
                    width_height_loaderCount_unloaderCount_agentCount_minSize);
                });
            });
        moveTO.wait();
    }


}


double computeSYCL(AlgorithmType which, Map& m) {
    internal::synchGPU(m);

    auto start_time = std::chrono::high_resolution_clock::now();
    switch (which)    {
    case AlgorithmType::ASTAR:
        internal::parallelAStar(m);
        break;
    default:
        break;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end_time - start_time).count();
}


std::string initializeSYCL(Map& m) {
    internal::q = new sycl::queue(sycl::default_selector_v);
    std::string res = internal::initializeSYCLMemory(m);
    if (res != "") {
        destroySYCL();
    }
    return res;
}

void destroySYCL() {
    internal::deleteGPUMem();
    internal::q = nullptr;
}
