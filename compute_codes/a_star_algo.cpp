#include "a_star_algo.h"
#include "map.h"

namespace internal {
    void initializeCostsAndHeap(int* gCost, int* fCost, bool* visited,
        MyHeap& myHeap, Position start, int mapSize, int width) {
        const int INF = 1e9;
        for (int i = 0; i < mapSize; i++) {
            gCost[i] = INF;
            fCost[i] = INF;
            visited[i] = false;
        }
        gCost[start.y * width + start.x] = 0;
        fCost[start.y * width + start.x] = 0;
        push(myHeap, start, fCost, width);
    }

    void runAStar(int* gCost, int* fCost, bool* visited, Position* cameFrom,
        char* grid, MyHeap& myHeap, Position start, Position goal, int width, int height) {
        while (!empty(myHeap)) {
            Position current = pop(myHeap, fCost, width);
            if (current.x == goal.x && current.y == goal.y) break;

            if (visited[current.y * width + current.x]) continue;
            visited[current.y * width + current.x] = true;

            Position neighbors[4] = {
                {current.x + 1, current.y}, {current.x - 1, current.y},
                {current.x, current.y + 1}, {current.x, current.y - 1} };

            for (int j = 0; j < 4; j++) {
                Position next = neighbors[j];
                if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                    grid[next.y * width + next.x] != OBSTACLE_SYMBOL && !visited[next.y * width + next.x]) {

                    int newG = gCost[current.y * width + current.x] + 1;
                    if (newG < gCost[next.y * width + next.x]) {
                        gCost[next.y * width + next.x] = newG;
                        fCost[next.y * width + next.x] = newG + abs(goal.x - next.x) + abs(goal.y - next.y);
                        cameFrom[next.y * width + next.x] = current;
                        push(myHeap, next, fCost, width);
                    }
                }
            }
        }
    }

    int reconstructPath(Position* paths, Position* cameFrom, Position start, Position goal, int width) {
        Position current = goal;
        int pathLength = 0;

        while (current.x != start.x || current.y != start.y) {
            paths[pathLength++] = current;
            current = cameFrom[current.y * width + current.x]; 
        }
        paths[pathLength++] = start;

        // Reverse path
        for (int i = 0; i < pathLength / 2; i++) {
            std::swap(paths[i], paths[pathLength - i - 1]);
        }

        return pathLength;
    }

    void computePathForAgent(int agentId, int* width_height_loaderCount_unloaderCount_agentCount_minSize,
        char* grid, Agent* agents, Position* loaderPosition, Position* unloaderPosition,
        Position* paths, int* pathSizes, int* fCost, int* gCost, bool* visited, Position* cameFrom, Position* openList) {
        int sizePath = pathSizes[agentId];
        if (sizePath != 0) {
            return;
        }
        const int width = width_height_loaderCount_unloaderCount_agentCount_minSize[WIDTHS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount_minSize[HEIGHTS_INDEX];
        const int mapSize = width * height;

        int offset = agentId * mapSize;
        int* fCostLocal = &fCost[offset];
        int* gCostLocal = &gCost[offset];
        bool* visitedLocal = &visited[offset];
        Position* cameFromLocal = &cameFrom[offset];
        Position* pathsLocal = &paths[offset];

        Agent a = agents[agentId];
        Position start = Position{a.x, a.y};
        Position goal = (a.direction == AGENT_LOADER) ? loaderPosition[a.loaderCurrent] : unloaderPosition[a.unloaderCurrent];

        MyHeap myHeap{ &openList[offset], 0 };
        initializeCostsAndHeap(gCostLocal, fCostLocal, visitedLocal, myHeap, start, mapSize, width);
        runAStar(gCostLocal, fCostLocal, visitedLocal, cameFromLocal, grid, myHeap, start, goal, width, height);
        pathSizes[agentId] = reconstructPath(pathsLocal, cameFromLocal, start, goal, width);
    }
}