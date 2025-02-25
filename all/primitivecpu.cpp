#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <algorithm> // Potrebné pre std::min_element
#include <barrier>
#include <thread> 

namespace internal {

    inline bool checkCollision(int agentId, int t, Position nextPos, int& conflictAgentId, Constrait& conflictFuturePos,
        Position* paths, int* pathSizes, int agentCount, int mapSize) {
        for (int agentId2 = 0; agentId2 < agentCount; agentId2++) {
            if (agentId == agentId2) continue;

            Position* pathOther = &paths[agentId2 * mapSize];
            int otherPathSize = pathSizes[agentId2];

            if (t >= otherPathSize - 1) continue;

            Position otherNextPos = pathOther[t + 1];

            if (otherNextPos.x == nextPos.x && otherNextPos.y == nextPos.y) {
                conflictAgentId = agentId2;
                conflictFuturePos.index = t;
                conflictFuturePos.x = otherNextPos.x;
                conflictFuturePos.y = otherNextPos.y;
                return true;
            }
        }
        return false;
    }

    inline bool shouldYield(int agentId, int conflictAgentId, int agentDirection, int otherDirection,
        int agentPathSize, int otherPathSize) {
        if (agentDirection != otherDirection) {
            return (agentDirection == AGENT_LOADER);
        }
        if (agentPathSize != otherPathSize) {
            return (agentPathSize > otherPathSize);
        }
        return (agentId > conflictAgentId);
    }

    inline int findAlternativePath(Position currentPos, Position targetPos, Position* foundPath,
        int width, int height, char* grid, Constrait* constraints, int numConstraints, int currentIndex) {
        const int MAX_PATH = 25;
        const int SEARCH_SIZE = 5;

        Position queue[MAX_PATH];
        int queueIndex[MAX_PATH];
        int queueStart = 0, queueEnd = 0;
        Position cameFrom[MAX_PATH];
        bool visited[SEARCH_SIZE][SEARCH_SIZE] = { false };

        queue[queueEnd] = currentPos;
        queueIndex[queueEnd] = currentIndex;
        visited[SEARCH_SIZE / 2][SEARCH_SIZE / 2] = true;
        queueEnd++;

        Position directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
        int foundPathSize = 0;

        while (queueStart < queueEnd) {
            Position current = queue[queueStart];
            int currentIdx = queueIndex[queueStart];
            queueStart++;

            if (current.x == targetPos.x && current.y == targetPos.y) {
                Position backtrack = current;
                while (backtrack.x != currentPos.x || backtrack.y != currentPos.y) {
                    foundPath[foundPathSize++] = backtrack;
                    for (int i = 0; i < queueEnd; i++) {
                        if (cameFrom[i].x == backtrack.x && cameFrom[i].y == backtrack.y) {
                            backtrack = queue[i];
                            break;
                        }
                    }
                }
                return foundPathSize;
            }

            for (int d = 0; d < 4; d++) {
                Position next = { current.x + directions[d].x, current.y + directions[d].y };
                int nextIndex = currentIdx + 1;

                if (next.x < 0 || next.x >= width || next.y < 0 || next.y >= height) continue;

                int localX = next.x - (currentPos.x - SEARCH_SIZE / 2);
                int localY = next.y - (currentPos.y - SEARCH_SIZE / 2);

                if (localX < 0 || localX >= SEARCH_SIZE || localY < 0 || localY >= SEARCH_SIZE) continue;
                if (visited[localY][localX]) continue;

                bool isBlocked = false;
                for (int c = 0; c < numConstraints; c++) {
                    if (constraints[c].x == next.x && constraints[c].y == next.y && constraints[c].index == nextIndex) {
                        isBlocked = true;
                        break;
                    }
                }
                if (isBlocked) continue;

                if (grid[next.y * width + next.x] == OBSTACLE_SYMBOL) continue;

                queue[queueEnd] = next;
                queueIndex[queueEnd] = nextIndex;
                cameFrom[queueEnd] = current;
                visited[localY][localX] = true;
                queueEnd++;
            }
        }
        return 0;
    }

    inline void applyPathShift(Position* paths, int agentSizePath, int t, Position* foundPath, int foundPathSize) {
        for (int moveIdx = agentSizePath - 1; moveIdx > t; moveIdx--) {
            paths[moveIdx + foundPathSize] = paths[moveIdx];
        }
        for (int i = 0; i < foundPathSize; i++) {
            paths[t + 1 + i] = foundPath[foundPathSize - 1 - i];
        }
    }

    bool handleCollision(int agentId, int conflictAgentId, unsigned char agentDirection, unsigned char conflictAgentDirection,
        int agentSizePath, int conflictAgentSizePath, Position* pathsLocal, int& agentSizePathRef,
        Position currentPos, Position nextGoal, Constrait* agentConstrait, int& agentConstraitSize,
        int width, int height, char* grid, int timeStep) {

        if (!shouldYield(agentId, conflictAgentId, agentDirection, conflictAgentDirection, agentSizePath, conflictAgentSizePath)) {
            return false;
        }

        agentConstrait[agentConstraitSize++] = { nextGoal.x, nextGoal.y, timeStep };

        Position foundPath[25];
        int foundPathSize = findAlternativePath(currentPos, nextGoal, foundPath, width, height, grid, agentConstrait, agentConstraitSize, timeStep);

        if (foundPathSize > 0) {
            applyPathShift(pathsLocal, agentSizePathRef, timeStep, foundPath, foundPathSize);
            agentSizePathRef += foundPathSize;
            return true;
        }

        return false;
    }

    void processAgentCollisionsGPU(sycl::nd_item<1> item, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents) {

        int agentId = item.get_local_id(0);  // Každý agent dostane svoje ID v rámci work-groupy

        const int width = width_height_loaderCount_unloaderCount_agentCount[WIDTHS_INDEX];
        const int agentCount = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount[HEIGHTS_INDEX];
        const unsigned char agentDirection = agents[agentId].direction;

        int offset = agentId * width * height;
        Position* pathsLocal = &paths[offset];
        int& agentSizePath = pathSizes[agentId];

        for (int t = 0; t < agentSizePath - 1; t++) {
            item.barrier();
            Position currentPos = pathsLocal[t];
            Position nextPos = pathsLocal[t + 1];

            int conflictAgentId = -1;
            Constrait conflictFuturePos = { -1, -1, -1 };

            if (!checkCollision(agentId, t, nextPos, conflictAgentId, conflictFuturePos, paths, pathSizes, agentCount, width * height)) {
                continue;
            }

            Constrait* agentConstrait = &constrait[agentId * (agentCount - 1)];
            int& agentConstraitSize = numberConstrait[agentId];

            handleCollision(agentId, conflictAgentId, agentDirection, agents[conflictAgentId].direction,
                agentSizePath, pathSizes[conflictAgentId], pathsLocal, agentSizePath, currentPos,
                nextPos, agentConstrait, agentConstraitSize, width, height, grid, t);
        }
        sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
            sycl::access::address_space::global_space>  atomicMin(*minSize);
        atomicMin.fetch_min(agentSizePath);
    }


    void processAgentCollisionsCPU(MyBarrier& b, int agentId, Position* paths, int* pathSizes,
        int* width_height_loaderCount_unloaderCount_agentCount, int* minSize, char* grid,
        Constrait* constrait, int* numberConstrait, Agent* agents) {

        const int width = width_height_loaderCount_unloaderCount_agentCount[WIDTHS_INDEX];
        const int agentCount = width_height_loaderCount_unloaderCount_agentCount[AGENTS_INDEX];
        const int height = width_height_loaderCount_unloaderCount_agentCount[HEIGHTS_INDEX];
        const unsigned char agentDirection = agents[agentId].direction;

        int offset = agentId * width * height;
        Position* pathsLocal = &paths[offset];
        int& agentSizePath = pathSizes[agentId];

        for (int t = 0; t < agentSizePath - 1; t++) {
            b.barrier();
            Position currentPos = pathsLocal[t];
            Position nextPos = pathsLocal[t + 1];

            int conflictAgentId = -1;
            Constrait conflictFuturePos = { -1, -1, -1 };

            if (!checkCollision(agentId, t, nextPos, conflictAgentId, conflictFuturePos, paths, pathSizes, agentCount, width * height)) {
                continue;
            }

            Constrait* agentConstrait = &constrait[agentId * (agentCount - 1)];
            int& agentConstraitSize = numberConstrait[agentId];

            handleCollision(agentId, conflictAgentId, agentDirection, agents[conflictAgentId].direction,
                agentSizePath, pathSizes[conflictAgentId], pathsLocal, agentSizePath, currentPos,
                nextPos, agentConstrait, agentConstraitSize, width, height, grid, t);
        }
        std::atomic<int>& atomicMin = *reinterpret_cast<std::atomic<int>*>(minSize);
        int oldValue = atomicMin.load();
        int newValue = agentSizePath;
        while (oldValue > newValue && !atomicMin.compare_exchange_weak(oldValue, newValue)) {}
    }
}
