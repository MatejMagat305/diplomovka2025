#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <algorithm> // Potrebné pre std::min_element
#include <thread> 
#include "fill_init_convert.h"

namespace internal {

    inline int max_path(MemoryPointers& localMemory) {
        int maxSize = 0;
        int agentCount = localMemory.agentCount;
        for (int i = 0; i < agentCount; i++) {
            int candidate = localMemory.agents[i].sizePath;
            if (maxSize < candidate ) {
                maxSize = candidate;
            }
        }
        return maxSize;
    }

    inline void writeMinimalPath(int agentId, MemoryPointers& localMemory) {
        if (agentId == 0) {
            int minSize = localMemory.agents[0].sizePath;
            for (int i = 1; i < localMemory.agentCount; i++) {
                int candidate = localMemory.agents[i].sizePath;
                if (candidate < minSize) {
                    minSize = candidate;
                }
            }
            *localMemory.minSize = minSize;
        }
    }

    inline void applyPathShift(Position* paths, int agentSizePath, int t, Position* foundPath, int foundPathSize) {
        for (int moveIdx = agentSizePath - 1; moveIdx > t; moveIdx--) {
            paths[moveIdx + foundPathSize] = paths[moveIdx];
        }
        for (int i = 0; i < foundPathSize; i++) {
            paths[t + 1 + i] = foundPath[foundPathSize - 1 - i];
        }
    }

    inline bool checkCollision(int agentId, int& conflictAgentId, int t, Constrait& conflictFuturePos, const MemoryPointers& globalMemory) {
        const int mapSize = globalMemory.width * globalMemory.height;
        Position* pathAgent = &globalMemory.pathsAgent[agentId * mapSize];
        int pathSize = globalMemory.agents[agentId].sizePath;
        Position nextPos = pathAgent[t + 1];

        for (int agentId2 = 0; agentId2 < globalMemory.agentCount; agentId2++) {
            if (agentId == agentId2) continue;
            Position* pathOther = &globalMemory.pathsAgent[agentId2 * mapSize];
            int otherPathSize = globalMemory.agents[agentId2].sizePath;

            if (t >= otherPathSize - 1) continue;

            Position otherNextPos = pathOther[t + 1];

            if (otherNextPos.x == nextPos.x && otherNextPos.y == nextPos.y) {
                conflictAgentId = agentId2;
                conflictFuturePos = { otherNextPos.x, otherNextPos.y, t };
                return true;
            }
        }
        return false;
    }

    inline bool shouldYield(int agentId, int conflictAgentId, Agent* agents) {
        Agent& a = agents[agentId], & b = agents[conflictAgentId];
        if (a.direction != b.direction) {
            return (a.direction == AGENT_LOADER);
        }
        if (a.sizePath != b.sizePath) {
            return (a.sizePath > b.sizePath);
        }
        return (agentId > conflictAgentId);
    }

    inline int runAStarContrait(Position start, Position goal, MemoryPointers& localMemory, int startTime, int agentID) {
        MyHeap myHeap{ localMemory.openList, 0 };
        initializeCostsAndHeap(localMemory, myHeap, start);

        while (!empty(myHeap)) {
            Position current = pop(myHeap, localMemory.fCost, localMemory.width);
            int trueIndex = getTrueIndexGrid(localMemory.width, current.x, current.y);
            int currentTime = localMemory.gCost[trueIndex]; // Použijeme G-cost ako časový krok

            if (current.x == goal.x && current.y == goal.y) break;
            if (localMemory.visited[trueIndex]) continue;

            localMemory.visited[trueIndex] = true;

            Position neighbors[4] = {
                {current.x + 1, current.y}, {current.x - 1, current.y},
                {current.x, current.y + 1}, {current.x, current.y - 1}
            };

            for (Position next : neighbors) {
                if (next.x < 0 || next.x >= localMemory.width || next.y < 0 || next.y >= localMemory.height) continue;

                int nextTrueIndex = getTrueIndexGrid(localMemory.width, next.x, next.y);
                if (localMemory.grid[nextTrueIndex] == OBSTACLE_SYMBOL || localMemory.visited[nextTrueIndex]) continue;

                // **KONTROLA CONSTRAINTOV**
                bool isBlocked = false;
                for (int c = 0; c < localMemory.numberConstrait[agentID]; c++) {  // Predpokladáme max constraints
                    if (localMemory.constrait[c].x == next.x &&
                        localMemory.constrait[c].y == next.y &&
                        localMemory.constrait[c].index == (currentTime + 1)) {  // Pozor na časový krok
                        isBlocked = true;
                        break;
                    }
                }
                if (isBlocked) continue;  // Preskočíme túto pozíciu, ak je zakázaná v čase

                int newG = currentTime + 1; // Každý pohyb zvýši časový krok
                if (newG < localMemory.gCost[nextTrueIndex]) {
                    localMemory.gCost[nextTrueIndex] = newG;
                    localMemory.fCost[nextTrueIndex] = newG + myAbs(goal.x - next.x) + myAbs(goal.y - next.y);
                    localMemory.cameFrom[nextTrueIndex] = current;
                    push(myHeap, next, localMemory.fCost, localMemory.width);
                }
            }
        }

        Position backtrack = goal;
        int pathSize = 0;
        while (!(backtrack.x == start.x && backtrack.y == start.y)) {
            localMemory.pathsAgent[pathSize++] = backtrack;
            backtrack = localMemory.cameFrom[getTrueIndexGrid(localMemory.width, backtrack.x, backtrack.y)];
        }
        return pathSize;
    }

    inline int solveCollision(int timeStep, MemoryPointers& localMemory, int agentId, bool& skip, const MemoryPointers& globalMemory) {
        const int mapSize = localMemory.width * localMemory.height;
        if (timeStep >= localMemory.agents[agentId].sizePath) {
            skip = true;
        }
        else {
            int conflictAgentId = -1;
            Constrait conflictFuturePos = { -1, -1, -1 };

            if (checkCollision(agentId, conflictAgentId, timeStep, conflictFuturePos, globalMemory)) {
                Position currentPos = localMemory.pathsAgent[agentId * mapSize + timeStep];
                Position nextGoal = localMemory.pathsAgent[agentId * mapSize + timeStep + 1];

                if (shouldYield(agentId, conflictAgentId, localMemory.agents)) {
                    int indexTopConstrait = localMemory.numberConstrait[agentId];
                    localMemory.constrait[indexTopConstrait] = { nextGoal.x, nextGoal.y, timeStep };
                    localMemory.numberConstrait[agentId] += 1;
                    return runAStarContrait(currentPos, nextGoal, localMemory, timeStep, agentId);
                }
            }
        }
        return 0;
    }

    inline void processAgentCollisionsGPU(sycl::nd_item<1> item, MemoryPointers& localMemory, const MemoryPointers& globalMemory) {
        int agentId = item.get_local_id(0);
        bool skip = false;
        int maxPath = max_path(localMemory);
        for (int timeStep = 0;; timeStep++) {
            if (timeStep >= maxPath){
                break;
            }
            int pathSize = 0;
            if (!skip) {
                pathSize = solveCollision(timeStep, localMemory, agentId, skip, globalMemory);
            }
            item.barrier();
            if (pathSize > 0) {
                Agent& a = localMemory.agents[agentId];
                applyPathShift(localMemory.pathsAgent, a.sizePath, timeStep, localMemory.pathsAgent, pathSize);
                int candidate = a.sizePath + pathSize;
                a.sizePath = candidate;
                if (candidate > maxPath) {
                    maxPath = candidate;
                }
            }
            item.barrier();
        }
        writeMinimalPath(agentId, localMemory);
    }

    void processAgentCollisionsCPU(MyBarrier& b, int agentId, MemoryPointers& localMemory, const MemoryPointers& globalMemory) {
        bool skip = false;
        for (int timeStep = 0;; timeStep++) {
            if (timeStep >= max_path(localMemory)) {
                break;
            }
            int pathSize = 0;
            if (!skip) {
                pathSize = solveCollision(timeStep, localMemory, agentId, skip, globalMemory);
            }
            b.barrier();
            if (pathSize > 0) {
                Agent& a = localMemory.agents[agentId];
                applyPathShift(localMemory.pathsAgent, a.sizePath, timeStep, localMemory.pathsAgent, pathSize);
                a.sizePath += pathSize;
            }
        }
        writeMinimalPath(agentId, localMemory);
    }
}
