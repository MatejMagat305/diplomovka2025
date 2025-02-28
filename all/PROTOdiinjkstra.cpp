/*

#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <sstream>

namespace internal {

    void parallelDijkstra(Map& m) {
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                int agentId = idx[0];
                const int width = width_height_loaderCount_unloaderCount_agentCount_minSize[WIDTHS_INDEX];
                const int height = width_height_loaderCount_unloaderCount_agentCount_minSize[HEIGHTS_INDEX];
                const int mapSize = width * height;

                int offset = agentId * mapSize;
                int* gCostLocal = &gCost[offset];
                bool* visitedLocal = &visited[offset];
                Position* cameFromLocal = &cameFrom[offset];
                Position* pathsLocal = &paths[offset];

                Agent a = agents[agentId];
                Position start = a.position;
                Position goal = unloaderPosition[a.unloaderCurrent];

                if (a.direction == AGENT_LOADER) {
                    goal = loaderPosition[a.loaderCurrent];
                }

                const int INF = 1e9;
                for (int i = 0; i < mapSize; i++) {
                    gCostLocal[i] = INF;
                    visitedLocal[i] = false;
                }

                gCostLocal[start.y * width + start.x] = 0;
                MyHeap myHeap{ &openList[offset], 0 };
                push(myHeap, start, gCostLocal, width);


                while (!empty(myHeap)) {
                    Position current = pop(myHeap, gCostLocal, width);

                    if (current.x == goal.x && current.y == goal.y) break;

                    if (visitedLocal[current.y * width + current.x]) continue;
                    visitedLocal[current.y * width + current.x] = true;

                    Position neighbors[4] = {
                        {current.x + 1, current.y}, {current.x - 1, current.y},
                        {current.x, current.y + 1}, {current.x, current.y - 1}
                    };

                    for (int j = 0; j < 4; j++) {
                        Position next = neighbors[j];
                        if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                            grid[next.y * width + next.x] != OBSTACLE_SYMBOL && !visitedLocal[next.y * width + next.x]) {

                            int newG = gCostLocal[current.y * width + current.x] + 1;

                            if (newG < gCostLocal[next.y * width + next.x]) {
                                gCostLocal[next.y * width + next.x] = newG;
                                cameFromLocal[next.y * width + next.x] = current;
                                push(myHeap, next, gCostLocal, width);
                            }
                        }
                    }
                }

                // Rekonštrukcia cesty
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
            h.parallel_for(sycl::nd_range<2>({ m.agentCount, maxTimeSteps }, { 1, 1 }),
                [=](sycl::id<2> idx) {
                    int agentId = idx[0];
                    int t = idx[1];

                    resolveAgentCollisionAtTime(agentId, t, paths, pathSizes, agents,
                        constrait, numberConstrait, grid,
                        width_height_loaderCount_unloaderCount_agentCount_minSize);
                });
            });
        collisionEvent.wait();

        q->memcpy(m.agentPaths, paths, m.agentCount * 100 * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }
}

*/