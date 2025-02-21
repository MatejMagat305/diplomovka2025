/*
//agent.h
#pragma once
#include <map>
#include <utility>
#include <vector>
#include <iostream>

#define AGENT_UNLOADER 1
#define AGENT_LOADER 2

struct Position {
    int x, y;
};

struct Agent {
    Position position = { 0,0 };
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};
//heap_primitive.h
#pragma once
#include "agent.h"

struct MyHeap {
    Position* heap;
    int size;
};

struct Constrait {
    int index, x, y;
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


//map.h

struct Constrait {
    int index,x,y;
};
#pragma once
#include <map>
#include <string>
#include "agent.h"
#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

struct Map {
    int width, height, loaderCount, unloaderCount, obstacleCount, agentCount;

    char** grid = nullptr;

    Position* loaderPosition = nullptr;
    Position* unloaderPosition = nullptr;

    int* agentPathSizes = nullptr;
    Position** agentPaths = nullptr;
    Agent* agents = nullptr;
};

// robim
#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>
#include "heap_primitive.h"

namespace internal {
#define WIDTHS_INDEX 0
#define HEIGHTS_INDEX 1
#define LOADERS_INDEX 2
#define UNLOADERS_INDEX 3
#define AGENTS_INDEX 4
#define MIN_INDEX 5
#define SIZE_INDEXES 6

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

        size_t globalMem = (*q).get_device().get_info<sycl::info::device::global_mem_size>();
        if (totalAlloc >= ((globalMem * 6) / 8)) {
            std::stringstream ss;
            ss << "you have not enough device memory" << totalAlloc << "/" << ((globalMem * 6) / 8);
            return ss.str();
        }
        try {
            width_height_loaderCount_unloaderCount_agentCount_minSize = sycl::malloc_device<int>(sizeof(int) * SIZE_INDEXES, (*q));
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

    void synchGPU(Map& m) {
        if (wasSynchronize()) {
            return;
        }
        setGPUSynch(true);
        int mapSize = m.width * m.height;
        (*q).memcpy(grid, m.grid, mapSize * sizeof(char)).wait();
        (*q).memcpy(loaderPosition, m.loaderPosition, m.loaderCount * sizeof(Position)).wait();
        (*q).memcpy(unloaderPosition, m.unloaderPosition, m.unloaderCount * sizeof(Position)).wait();
        (*q).memcpy(agents, m.agents, m.agentCount * sizeof(Agent)).wait();
        int temp[SIZE_INDEXES] = { m.width, m.height, m.loaderCount, m.unloaderCount, m.agentCount, std::numeric_limits<int>::max() };
        (*q).memcpy(width_height_loaderCount_unloaderCount_agentCount_minSize, temp, SIZE_INDEXES * sizeof(int)).wait();
    }

    inline void freePtr(void* ptr) {
        if (ptr != nullptr) {
            sycl::free(ptr, *q);
        }
    }

    void deleteGPUMem() {
        freePtr(width_height_loaderCount_unloaderCount_agentCount_minSize);
        freePtr((void*)(unloaderPosition));
        freePtr((void*)(loaderPosition));
        freePtr((void*)(grid));
        freePtr((void*)(agents));

        freePtr((void*)(visited));
        freePtr((void*)(cameFrom));
        freePtr((void*)(fCost));
        freePtr((void*)(gCost));
        freePtr((void*)(openList));
        freePtr((void*)(paths));
        freePtr((void*)(pathSizes));
    }


    void deleteCPUStruct() {
        delete q;
        q = nullptr;
    }



    void parallelAStar(Map& m) {
        auto e = (*q).submit([&](sycl::handler& h) {
                // do search 
                // .... - toto mám
            });

        // Synchronizácia výsledkov s CPU
        q->memcpy(m.agentPaths, paths, m.agentCount * m.height * m.width * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }
}
*/