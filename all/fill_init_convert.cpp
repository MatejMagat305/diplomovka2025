#include "fill_init_convert.h"

inline int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}

inline int myAbs(int input) {
    if (input < 0) {
        return -input;
    }
    return input;
}

namespace internal {
    inline void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory) {
        localMemory.width = globalMemory.width;
        localMemory.height = globalMemory.height;
        localMemory.agentCount = globalMemory.agentCount;
        localMemory.loaderCount = globalMemory.loaderCount;
        localMemory.unloaderCount = globalMemory.unloaderCount;

        const int mapSize = localMemory.width * localMemory.height;
        int offset = mapSize * agentId;
        localMemory.grid = globalMemory.grid;
        localMemory.loaderPosition = globalMemory.loaderPosition;
        localMemory.unloaderPosition = globalMemory.unloaderPosition;
        localMemory.agents = globalMemory.agents;
        localMemory.minSize = globalMemory.minSize;
        localMemory.pathsAgent = &(globalMemory.pathsAgent[offset]);
        localMemory.gCost = &(globalMemory.gCost[offset]);
        localMemory.fCost = &(globalMemory.fCost[offset]);
        localMemory.cameFrom = &(globalMemory.cameFrom[offset]);
        localMemory.visited = &(globalMemory.visited[offset]);
        localMemory.openList = &(globalMemory.openList[offset]);
        localMemory.constrait = &(globalMemory.constrait[offset / 4]);
        localMemory.numberConstrait = globalMemory.numberConstrait;
    }

    inline void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start) {
        const int width = localMemory.width;
        const int height = localMemory.height;
        const int mapSize = width * height;
        const int INF = 1e9;
        for (int i = 0; i < mapSize; i++) {
            localMemory.gCost[i] = INF;
            localMemory.fCost[i] = INF;
            localMemory.visited[i] = false;
        }
        localMemory.gCost[getTrueIndexGrid(width, start.x, start.y)] = 0;
        localMemory.fCost[getTrueIndexGrid(width, start.x, start.y)] = 0;
        push(myHeap, start, localMemory.fCost, width);
    }
}