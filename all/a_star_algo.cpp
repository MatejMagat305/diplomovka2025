#include "a_star_algo.h"
#include "fill_init_convert.h"


namespace internal {

    inline void runAStar(MemoryPointers& localMemory, MyHeap& myHeap, Position start, Position goal) {
        const int width = localMemory.width;
        const int height = localMemory.height;
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
                        localMemory.fCost[getTrueIndexGrid(width, current.x, current.y)] = newG + myAbs(goal.x - next.x) + myAbs(goal.y - next.y);
                        localMemory.cameFrom[getTrueIndexGrid(width, current.x, current.y)] = current;
                        push(myHeap, next, localMemory.fCost, width);
                    }
                }
            }
        }
    }

    inline int reconstructPath(MemoryPointers& localMemory, Position start, Position goal) {
        const int width = localMemory.width;
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

    inline void computePathForAgent(int agentId, MemoryPointers& localMemory) {
        if (localMemory.agents[agentId].sizePath != 0) {
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