#include "compute.h"
#include <sycl/sycl.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>

namespace internal {
    #define WIDTHS_INDEX = 0;
    #define HEIGHTS_INDEX = 1;
    #define LOADERS_INDEX = 2;
    #define UNLOADERS_INDEX = 3;
    #define AGENTS_INDEX = 4;

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
    };

    struct Conflict {
        int agent1, agent2;
        Position pos;
        int index;
    };

    struct CBSNode {
        std::vector<std::vector<Position>> paths;
        int cost;
    };

    struct CompareCBS {
        bool operator()(const CBSNode& a, const CBSNode& b) {
            return a.cost > b.cost;
        }
    };


    MapDataGPU* dataGPU = nullptr;
    sycl::queue* q = nullptr;

    void initializeSYCLMemory(sycl::queue& q, Map& m, MapDataGPU& d_m) {
        int mapSize = m.width * m.height;
        int stackSize = m.agentCount * mapSize;
        d_m.width_height_loaderCount_unloaderCount_agentCount = sycl::malloc_device<int>(sizeof(int)*5, q);
        d_m.grid = sycl::malloc_device<char>(m.width * m.height, q);
        d_m.loaderPosition = sycl::malloc_device<Position>(m.loaderCount, q);
        d_m.unloaderPosition = sycl::malloc_device<Position>(m.unloaderCount, q);
        d_m.paths = sycl::malloc_device<Position>(stackSize, q);
        d_m.pathSizes = sycl::malloc_device<int>(m.agentCount, q);
        d_m.gCost = sycl::malloc_device<int>(stackSize, q);
        d_m.fCost = sycl::malloc_device<int>(stackSize, q);
        d_m.cameFrom = sycl::malloc_device<Position>(stackSize, q);
        d_m.visited = sycl::malloc_device<bool>(stackSize, q);
        d_m.openList = sycl::malloc_shared<Position>(stackSize, q);
        q.memcpy(d_m.grid, m.grid, mapSize * sizeof(char));
        q.memcpy(d_m.loaderPosition, m.loaderPosition, m.loaderCount * sizeof(Position));
        q.memcpy(d_m.unloaderPosition, m.unloaderPosition, m.unloaderCount * sizeof(Position));
        q.memcpy(d_m.agents, m.agents, m.agentCount * sizeof(Agent));
        int temp[5] = {m.width, m.height, m.loaderCount, m.unloaderCount, m.agentCount};
        q.memcpy(d_m.width_height_loaderCount_unloaderCount_agentCount, temp, 5 * sizeof(int));
        q.wait();
    }

    void deleteGPUMem() {
        sycl::free(dataGPU->width_height_loaderCount_unloaderCount_agentCount, *q);
        sycl::free(dataGPU->unloaderPosition, *q);
        sycl::free(dataGPU->loaderPosition, *q);
        sycl::free(dataGPU->grid, *q);
        sycl::free(dataGPU->agents, *q);

        sycl::free(dataGPU->visited, *q);
        sycl::free(dataGPU->cameFrom, *q);
        sycl::free(dataGPU->fCost, *q);
        sycl::free(dataGPU->gCost, *q);
        sycl::free(dataGPU->openList, *q);

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
        auto width_height_loaderCount_unloaderCount_agentCount = dataGPU->width_height_loaderCount_unloaderCount_agentCount;
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                int agentId = idx[0];
                const int width = width_height_loaderCount_unloaderCount_agentCount[0];
                const int height = width_height_loaderCount_unloaderCount_agentCount[1];
                const int mapSize = width * height;

                int offset = agentId * mapSize;  // Každý agent má svoj blok v 1D poli

                Agent a = agents[agentId];  // Použiť hodnotu, nie referenciu
                Position start = a.position;
                Position goal = unloaderPosition[a.unloaderCurrent];

                if (a.direction == AGENT_LOADER) {
                    goal = loaderPosition[a.loaderCurrent];
                }

                const int INF = 1e9;

                for (int i = 0; i < mapSize; i++) {
                    gCost[offset + i] = INF;
                    fCost[offset + i] = INF;
                    visited[offset + i] = false;
                }

                gCost[offset + start.y * width + start.x] = 0;
                fCost[offset + start.y * width + start.x] = 0;

                int openTop = offset;
                openList[openTop++] = start;

                while (openTop > offset) {
                    int bestIndex = 0;
                    for (int i = 1; i < openTop; i++) {
                        if (fCost[offset + openList[i].y * width + openList[i].x] <
                            fCost[offset + openList[bestIndex].y * width + openList[bestIndex].x]) {
                            bestIndex = i;
                        }
                    }

                    Position current = openList[bestIndex];
                    openList[bestIndex] = openList[openTop];
                    openTop--;

                    if (current.x == goal.x && current.y == goal.y) break;

                    if (visited[offset + current.y * width + current.x]) continue;
                    visited[offset + current.y * width + current.x] = true;

                    Position neighbors[4] = {
                        {current.x + 1, current.y}, {current.x - 1, current.y},
                        {current.x, current.y + 1}, {current.x, current.y - 1} };

                    for (int j = 0;j<4;j++) {
                        Position next = neighbors[j];
                        if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                            grid[next.y * width + next.x] != OBSTACLE_SYMBOL && !visited[offset + next.y * width + next.x]) {

                            int newG = gCost[offset + current.y * width + current.x] + 1;
                            if (newG < gCost[offset + next.y * width + next.x]) {
                                gCost[offset + next.y * width + next.x] = newG;
                                fCost[offset + next.y * width + next.x] = newG + abs(goal.x - next.x) + abs(goal.y - next.y);
                                cameFrom[offset + next.y * width + next.x] = current;
                                openList[openTop++] = next;
                            }
                        }
                    }
                }

                Position current = goal;
                int pathLength = 0;

                while (current.x != start.x || current.y != start.y) {
                    paths[offset + pathLength] = current;
                    pathLength++;
                    current = cameFrom[offset + current.y * width + current.x];
                }
                paths[pathLength] = start;
                pathLength++;
                // reverse
                pathSizes[agentId] = pathLength;
                for (int i = 0; i < pathLength/2; i++) {
                    Position temp = paths[offset + i];
                    paths[offset + i] = paths[offset + pathLength - 1 - i];
                    paths[offset + pathLength - 1 - i] = temp;
                }
                });
            });


        // kolízie
        (*q).submit([&](sycl::handler& h) {h.depends_on(e);});

        // Skopírovanie výsledkov do CPU
        q->memcpy(m.agentPaths, paths, m.agentCount * 100 * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }

    void parallelDijkstra(Map& m) {
        auto width_height_loaderCount_unloaderCount_agentCount = dataGPU->width_height_loaderCount_unloaderCount_agentCount;
        auto unloaderPosition = dataGPU->unloaderPosition;
        auto loaderPosition = dataGPU->loaderPosition;
        auto grid = dataGPU->grid;
        auto agents = dataGPU->agents;
        auto paths = dataGPU->paths;
        auto pathSizes = dataGPU->pathSizes;
        auto gCost = dataGPU->gCost;
        auto cameFrom = dataGPU->cameFrom;
        auto visited = dataGPU->visited;
        auto openList = dataGPU->openList;
        auto width_height_loaderCount_unloaderCount_agentCount = dataGPU->width_height_loaderCount_unloaderCount_agentCount;
        auto e = (*q).submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(m.agentCount), [=](sycl::id<1> idx) {
                int agentId = idx[0];
                const int width = width_height_loaderCount_unloaderCount_agentCount[0];
                const int height = width_height_loaderCount_unloaderCount_agentCount[1];
                const int mapSize = width * height;

                int offset = agentId * mapSize;

                Agent a = agents[agentId];
                Position start = a.position;
                Position goal = unloaderPosition[a.unloaderCurrent];

                if (a.direction == AGENT_LOADER) {
                    goal = loaderPosition[a.loaderCurrent];
                }

                const int INF = 1e9;
                for (int i = 0; i < mapSize; i++) {
                    gCost[offset + i] = INF;
                    visited[offset + i] = false;
                }

                gCost[offset + start.y * width + start.x] = 0;

                int openTop = offset;
                openList[openTop++] = start;

                while (openTop > offset) {
                    // Používame buckets -> nájdeme najmenší `gCost`
                    int minIndex = offset;
                    int minCost = INF;
                    for (int i = offset; i < openTop; i++) {
                        int cost = gCost[offset + openList[i].y * width + openList[i].x];
                        if (cost < minCost) {
                            minCost = cost;
                            minIndex = i;
                        }
                    }

                    Position current = openList[minIndex];
                    openList[minIndex] = openList[--openTop];

                    if (current.x == goal.x && current.y == goal.y) break;

                    if (visited[offset + current.y * width + current.x]) continue;
                    visited[offset + current.y * width + current.x] = true;

                    Position neighbors[4] = {
                        {current.x + 1, current.y}, {current.x - 1, current.y},
                        {current.x, current.y + 1}, {current.x, current.y - 1}
                    };

                    for (int j = 0; j < 4; j++) {
                        Position next = neighbors[j];
                        if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                            grid[next.y * width + next.x] != OBSTACLE_SYMBOL && !visited[offset + next.y * width + next.x]) {

                            int newG = gCost[offset + current.y * width + current.x] + 1;

                            if (newG < gCost[offset + next.y * width + next.x]) {
                                gCost[offset + next.y * width + next.x] = newG;
                                cameFrom[offset + next.y * width + next.x] = current;
                                openList[openTop++] = next;
                            }
                        }
                    }
                }

                // Rekonštrukcia cesty
                Position current = goal;
                int pathLength = 0;

                while (current.x != start.x || current.y != start.y) {
                    paths[offset + pathLength] = current;
                    pathLength++;
                    current = cameFrom[offset + current.y * width + current.x];
                }
                paths[pathLength] = start;
                pathLength++;

                pathSizes[agentId] = pathLength;
                for (int i = 0; i < pathLength / 2; i++) {
                    Position temp = paths[offset + i];
                    paths[offset + i] = paths[offset + pathLength - 1 - i];
                    paths[offset + pathLength - 1 - i] = temp;
                }
                });
            });

        (*q).submit([&](sycl::handler& h) { h.depends_on(e); });

        q->memcpy(m.agentPaths, paths, m.agentCount * 100 * sizeof(Position)).wait();
        q->memcpy(m.agentPathSizes, pathSizes, m.agentCount * sizeof(int)).wait();
    }

}


double computeSYCL(AlgorithmType which, Map& m) {
    // SYCL kód so zariadením
    return 0.0;
}


void initializeSYCL(Map& m) {
    internal::q = new sycl::queue(sycl::default_selector_v);
    internal::dataGPU = new internal::MapDataGPU();
    internal::initializeSYCLMemory(*internal::q, m, *internal::dataGPU);
}

void destroySYCL() {
    delete internal::q;
    internal::deleteGPUMem();
    delete internal::dataGPU;
}
