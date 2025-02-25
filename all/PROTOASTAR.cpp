/*
#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>

namespace internal {
#define WIDTHS_INDEX 0
#define HEIGHTS_INDEX 1
#define LOADERS_INDEX 2
#define UNLOADERS_INDEX 3
#define AGENTS_INDEX 4


    struct MapDataGPU {
        int* width_height_loaderCount_unloaderCount_agentCount;
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
        Position* constrait = nullptr;
    };
    struct MyHeap {
        Position* heap;
        int size;
    };

    void push(MyHeap& myHeap, Position p, int* gCost, int width) {
        myHeap.heap[myHeap.size] = p;
        myHeap.size++;
        int i = myHeap.size - 1;
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (gCost[myHeap.heap[parent].y * width + myHeap.heap[parent].x] <=
                gCost[myHeap.heap[i].y * width + myHeap.heap[i].x]) {
                break;
            }

            std::swap(myHeap.heap[parent], myHeap.heap[i]);
            i = parent;
        }
    }

    Position pop(MyHeap& myHeap, int* gCost, int width) {
        if (myHeap.size == 0) {
            return { -1, -1 };
        }
        Position root = myHeap.heap[0];
        --(myHeap.size);
        myHeap.heap[0] = myHeap.heap[myHeap.size];

        int i = 0;
        while (2 * i + 1 < myHeap.size) {
            int left = 2 * i + 1, right = 2 * i + 2;
            int smallest = left;

            if (right < myHeap.size &&
                gCost[myHeap.heap[right].y * width + myHeap.heap[right].x] <
                gCost[myHeap.heap[left].y * width + myHeap.heap[left].x]) {
                smallest = right;
            }

            if (gCost[myHeap.heap[i].y * width + myHeap.heap[i].x] <=
                gCost[myHeap.heap[smallest].y * width + myHeap.heap[smallest].x]) {
                break;
            }

            std::swap(myHeap.heap[i], myHeap.heap[smallest]);
            i = smallest;
        }

        return root;
    }

    bool empty(MyHeap& myHeap) {
        return myHeap.size == 0;
    }


    struct Conflict {
        int agent1, agent2;
        Position pos;
        int index;
    };


    MapDataGPU* dataGPU = nullptr;
    sycl::queue* q = nullptr;


    std::string initializeSYCLMemory(sycl::queue& q, Map& m, MapDataGPU& d_m) {
        size_t mapSize = m.width * m.height;
        size_t stackSize = m.agentCount * mapSize;

        size_t totalAlloc = 0;
        totalAlloc += 5 * sizeof(int); // width_height_loaderCount_unloaderCount_agentCount
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
        totalAlloc += stackSize * sizeof(Position); // constrait

        size_t globalMem = q.get_device().get_info<sycl::info::device::global_mem_size>();
        if (totalAlloc >= ((globalMem * 6) / 8)) {
            std::stringstream ss;
            ss << "you have not enough device memory" << totalAlloc << "/" << ((globalMem * 6) / 8);
            return ss.str();
        }
        try {
            int mapSize = m.width * m.height;
            int stackSize = m.agentCount * mapSize;
            d_m.width_height_loaderCount_unloaderCount_agentCount = sycl::malloc_device<int>(sizeof(int) * 5, q);
            d_m.grid = sycl::malloc_device<char>(mapSize, q);
            d_m.agents = sycl::malloc_device<Agent>(m.agentCount, q);
            d_m.loaderPosition = sycl::malloc_device<Position>(m.loaderCount, q);
            d_m.unloaderPosition = sycl::malloc_device<Position>(m.unloaderCount, q);
            d_m.paths = sycl::malloc_device<Position>(stackSize, q);
            d_m.pathSizes = sycl::malloc_device<int>(m.agentCount, q);
            d_m.gCost = sycl::malloc_device<int>(stackSize, q);
            d_m.fCost = sycl::malloc_device<int>(stackSize, q);
            d_m.cameFrom = sycl::malloc_device<Position>(stackSize, q);
            d_m.visited = sycl::malloc_device<bool>(stackSize, q);
            d_m.openList = sycl::malloc_device<Position>(stackSize, q);
            d_m.constrait = sycl::malloc_device<Position>(stackSize, q);
        }
        catch (sycl::exception& e) {
            std::stringstream ss;
            auto& category = e.category();
            ss << e.what();
            return ss.str();
        }
        return "";
    }

    void synchGPU(sycl::queue& q, MapDataGPU& d_m, Map& m) {
        if (wasSynchronize()) {
            return;
        }
        setGPUSynch(true);
        int mapSize = m.width * m.height;
        q.memcpy(d_m.grid, m.grid, mapSize * sizeof(char)).wait();
        q.memcpy(d_m.loaderPosition, m.loaderPosition, m.loaderCount * sizeof(Position)).wait();
        q.memcpy(d_m.unloaderPosition, m.unloaderPosition, m.unloaderCount * sizeof(Position)).wait();
        q.memcpy(d_m.agents, m.agents, m.agentCount * sizeof(Agent)).wait();
        int temp[5] = { m.width, m.height, m.loaderCount, m.unloaderCount, m.agentCount };
        q.memcpy(d_m.width_height_loaderCount_unloaderCount_agentCount, temp, 5 * sizeof(int)).wait();
    }

    inline void freePtr(void* ptr) {
        if (ptr != nullptr) {
            sycl::free(ptr, *q);
        }
    }

    void deleteGPUMem() {
        if (!dataGPU) return; // Overenie, či dataGPU existuje

        freePtr(dataGPU->width_height_loaderCount_unloaderCount_agentCount);
        freePtr(dataGPU->unloaderPosition);
        freePtr(dataGPU->loaderPosition);
        freePtr(dataGPU->grid);
        freePtr(dataGPU->agents);

        freePtr(dataGPU->visited);
        freePtr(dataGPU->cameFrom);
        freePtr(dataGPU->fCost);
        freePtr(dataGPU->gCost);
        freePtr(dataGPU->openList);
        freePtr(dataGPU->paths);
        freePtr(dataGPU->pathSizes);
    }


    void deleteCPUStruct() {
        delete dataGPU;
        delete q;
        dataGPU = nullptr;
        q = nullptr;
    }

    void parallelAStar(Map& m) {
        auto width_height_loaderCount_unloaderCount_agentCount = dataGPU->width_height_loaderCount_unloaderCount_agentCount;
        auto unloaderPosition = dataGPU->unloaderPosition;
        auto loaderPosition = dataGPU->loaderPosition;
        auto grid = dataGPU->grid;
        auto agents = dataGPU->agents;
        auto paths = dataGPU->paths;
        auto pathSizes = dataGPU->pathSizes;
        auto gCost = dataGPU->gCost;
        auto fCost = dataGPU->fCost;
        auto cameFrom = dataGPU->cameFrom;
        auto visited = dataGPU->visited;
        auto openList = dataGPU->openList;
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                int agentId = idx[0];
                const int width = width_height_loaderCount_unloaderCount_agentCount[WIDTHS_INDEX];
                const int height = width_height_loaderCount_unloaderCount_agentCount[HEIGHTS_INDEX];
                const int mapSize = width * height;

                int offset = agentId * mapSize;  // Každý agent má svoj blok v 1D poli
                int* fCostLocal = &fCost[offset];
                int* gCostLocal = &gCost[offset];
                bool* visitedLocal = &visited[offset];
                Position* cameFromLocal = &cameFrom[offset];
                Position* pathsLocal = &paths[offset];
                Agent a = agents[agentId];  // Použiť hodnotu, nie referenciu
                Position start = a.position;
                Position goal = unloaderPosition[a.unloaderCurrent];

                if (a.direction == AGENT_LOADER) {
                    goal = loaderPosition[a.loaderCurrent];
                }

                const int INF = 1e9;

                for (int i = 0; i < mapSize; i++) {
                    gCostLocal[i] = INF;
                    fCostLocal[i] = INF;
                    visitedLocal[i] = false;
                }

                gCostLocal[start.y * width + start.x] = 0;
                fCostLocal[start.y * width + start.x] = 0;
                MyHeap myHeap{ &openList[offset], 0 };
                push(myHeap, start, fCostLocal, width);


                while (!empty(myHeap)) {
                    Position current = pop(myHeap, fCostLocal, width);
                    if (current.x == goal.x && current.y == goal.y) break;

                    if (visitedLocal[current.y * width + current.x]) continue;
                    visitedLocal[current.y * width + current.x] = true;

                    Position neighbors[4] = {
                        {current.x + 1, current.y}, {current.x - 1, current.y},
                        {current.x, current.y + 1}, {current.x, current.y - 1} };

                    for (int j = 0;j < 4;j++) {
                        Position next = neighbors[j];
                        if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                            grid[next.y * width + next.x] != OBSTACLE_SYMBOL && !visitedLocal[next.y * width + next.x]) {

                            int newG = gCostLocal[current.y * width + current.x] + 1;
                            if (newG < gCostLocal[next.y * width + next.x]) {
                                gCostLocal[next.y * width + next.x] = newG;
                                fCostLocal[next.y * width + next.x] = newG + abs(goal.x - next.x) + abs(goal.y - next.y);
                                cameFromLocal[next.y * width + next.x] = current;
                                push(myHeap, next, fCostLocal, width);
                            }
                        }
                    }
                }

                Position current = goal;
                int pathLength = 0;

                while (current.x != start.x || current.y != start.y) {
                    pathsLocal[pathLength] = current;
                    pathLength++;
                    current = cameFromLocal[current.y * width + current.x];
                }
                pathsLocal[pathLength] = start;
                pathLength++;
                pathSizes[agentId] = pathLength;
                for (int i = 0; i < pathLength / 2; i++) {
                    Position temp = pathsLocal[i];
                    pathsLocal[i] = pathsLocal[pathLength - i - 1];
                    pathsLocal[pathLength - i - 1] = temp;
                }
                });
            });
        auto collisionEvent = (*q).submit([&](sycl::handler& h) {
            h.depends_on(e);
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                int agentId = idx[0];
                const int width = width_height_loaderCount_unloaderCount_agentCount[WIDTHS_INDEX];
                const int height = width_height_loaderCount_unloaderCount_agentCount[HEIGHTS_INDEX];
                const int mapSize = width * height;

                int offset = agentId * mapSize;
                Position* pathsLocal = &paths[offset];
                int agentSizePath = pathSizes[agentId];

                for (int t = 0; t < agentSizePath - 1; t++) {
                    Position currentPos = pathsLocal[t];
                    Position nextPos = pathsLocal[t + 1];

                    // 2️⃣ Kontrola kolízie
                    bool collisionDetected = false;
                    for (int agentId2 = 0; agentId2 < m.agentCount; agentId2++) {
                        if (agentId == agentId2) continue;
                        Position* pathOther = &paths[agentId2 * mapSize];
                        if (pathOther[t + 1].x == nextPos.x && pathOther[t + 1].y == nextPos.y) {
                            // pridať "obmedzenie" agentovi s nižším ID (ak mám nižšie ID, tak ...)
                            // ....

                            collisionDetected = true;
                            break;
                        }
                    }

                    if (!collisionDetected) continue; // Ak nie je kolízia, pokračujeme normálne

                    // 1️⃣ Pole pre alternatívnu trasu (max 25 krokov, pre stvorec 5x5)
                    Position stack[25];
                    int stackSize = 0;
                    if (agentSizePath <= t + 2) { // stretnú sa v ciely

                    }
                    else if ((agents[agentId].direction == AGENT_UNLOADER && (agents[agentId2].direction == AGENT_UNLOADER)) {
                        // 3️⃣ Generovanie alternatívnej cesty do `stack`
                        Position goalPos = pathsLocal[t + 2]; // Cieľ
                        findAlternativePathSYCL(currentPos, goalPos, width, height, stack, stackSize);

                    }


                    if (stackSize > 0) {
                        // 4️⃣ Posun starých dát, aby sme vložili novú trasu
                        int newSize = agentSizePath + stackSize - 1;
                        for (int moveIdx = agentSizePath - 1; moveIdx > t; moveIdx--) {
                            pathsLocal[moveIdx + stackSize - 1] = pathsLocal[moveIdx];
                        }

                        // 5️⃣ Vloženie alternatívnych pozícií
                        for (int i = 0; i < stackSize; i++) {
                            pathsLocal[t + 1 + i] = stack[i];
                        }

                        pathSizes[agentId] = newSize; // Aktualizácia dĺžky
                    }
                }
                        });
                });
        collisionEvent.wait();

        // Skopírovanie výsledkov do CPU
        q->memcpy(m.agentPaths, paths, m.agentCount * 100 * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }
}

*/