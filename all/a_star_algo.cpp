#include "a_star_algo.h"
#include "map.h"

int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}

namespace internal {

    inline void createLocalMemory(MemoryPointers& memory, int agentId, MemoryPointers& local) {
        const int width = memory.stuffWHLUA[WIDTHS_INDEX];
        const int height = memory.stuffWHLUA[HEIGHTS_INDEX];
        const int agentCount = memory.stuffWHLUA[AGENTS_INDEX];
        const int mapSize = width * height;
        int offset = mapSize * agentId;
         local = {
            memory.grid,
            memory.loaderPosition,
            memory.unloaderPosition,
            memory.agents,
            memory.stuffWHLUA,
            memory.minSize,
            memory.agents,
            &(memory.pathsAgent[offset]),
            memory.pathSizesAgent,
            &(memory.gCost[offset]),
            &(memory.fCost[offset]),
            &(memory.cameFrom[offset]),
            &(memory.visited[offset]),
            &(memory.openList[offset]),
            & (memory.constrait[agentId*(agentCount-1)]),
            memory.numberConstrait
        };
    }

    inline void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start) {
        const int width = localMemory.stuffWHLUA[WIDTHS_INDEX];
        const int height = localMemory.stuffWHLUA[HEIGHTS_INDEX];
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

    void runAStar(MemoryPointers& localMemory, MyHeap& myHeap, Position start, Position goal) {
        const int width = localMemory.stuffWHLUA[WIDTHS_INDEX];
        const int height = localMemory.stuffWHLUA[HEIGHTS_INDEX];
        while (!empty(myHeap)) {
            Position current = pop(myHeap, localMemory.fCost, width);
            if (current.x == goal.x && current.y == goal.y) break;

            if (localMemory.visited[getTrueIndexGrid(width, current.x, current.y)]) continue;
            localMemory.visited[getTrueIndexGrid(width, current.x, current.y)] = true;

            Position neighbors[4] = {
                {current.x + 1, current.y}, {current.x - 1, current.y},
                {current.x, current.y + 1}, {current.x, current.y - 1} };

            for (int j = 0; j < 4; j++) {
                Position next = neighbors[j];
                if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                    localMemory.grid[getTrueIndexGrid(width, current.x, current.y)] != OBSTACLE_SYMBOL && !localMemory.visited[getTrueIndexGrid(width, current.x, current.y)]) {

                    int newG = localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)] + 1;
                    if (newG < localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)]) {
                        localMemory.gCost[getTrueIndexGrid(width, current.x, current.y)] = newG;
                        localMemory.fCost[getTrueIndexGrid(width, current.x, current.y)] = newG + abs(goal.x - next.x) + abs(goal.y - next.y);
                        localMemory.cameFrom[getTrueIndexGrid(width, current.x, current.y)] = current;
                        push(myHeap, next, localMemory.fCost, width);
                    }
                }
            }
        }
    }

    int reconstructPath(MemoryPointers& localMemory, Position start, Position goal) {
        const int width = localMemory.stuffWHLUA[WIDTHS_INDEX];
        Position current = goal;
        int pathLength = 0;

        while (current.x != start.x || current.y != start.y) {
            localMemory.pathsAgent[pathLength++] = current;
            current = localMemory.cameFrom[getTrueIndexGrid(width, current.x, current.y)];
        }
        localMemory.pathsAgent[pathLength++] = start;

        // Reverse path
        for (int i = 0; i < pathLength / 2; i++) {
            Position temp = localMemory.pathsAgent[i];
            localMemory.pathsAgent[i] = localMemory.pathsAgent[pathLength - i - 1];
            localMemory.pathsAgent[pathLength - i - 1] = temp;
        }

        return pathLength;
    }

    void computePathForAgent(int agentId, MemoryPointers& localMemory) {
        int sizePath = localMemory.agents[agentId].sizePath;
        if (sizePath != 0) {
            return;
        }
        Agent a = localMemory.agents[agentId];
        Position start = Position{a.x, a.y};
        Position goal;
        if (a.direction == AGENT_LOADER)   {
            goal = localMemory.loaderPosition[a.loaderCurrent];
        }else {
            goal = localMemory.unloaderPosition[a.unloaderCurrent];
        }
        MyHeap myHeap{ localMemory.openList, 0 };
        initializeCostsAndHeap(localMemory, myHeap, start);
        runAStar(localMemory, myHeap, start, goal);
        localMemory.agents[agentId].sizePath = reconstructPath(localMemory, start, goal);
    }
}