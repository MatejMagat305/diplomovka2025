#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <thread> 
#include "fill_init_convert.h"
#include "d_star_algo.h"

int max_path(MemoryPointers& localMemory) {
    int maxSize = localMemory.agents[0].sizePath;
    int agentCount = localMemory.agentsCount;
    for (int i = 1; i < agentCount; i++) {
        int candidate = localMemory.agents[i].sizePath;
        if (maxSize < candidate) {
            maxSize = candidate;
        }
    }
    return maxSize;
}

void writeMinimalPath(const MemoryPointers& globalMemory) {
    int minSize = globalMemory.agents[0].sizePath;
    for (int i = 1; i < globalMemory.agentsCount; i++) {
        int candidate = globalMemory.agents[i].sizePath;
        if (candidate < minSize) {
            minSize = candidate;
        }
    }
    *globalMemory.minSize_numberColision = minSize;
}

bool checkCollision(int agentId, int& conflictAgentId, int t, Constrait& conflictFuturePos, const MemoryPointers& globalMemory) {
    const int mapSize = globalMemory.width * globalMemory.height;
    Position* pathAgent = &globalMemory.agentPaths[agentId * mapSize];
    Position nextPos = pathAgent[t + 1];
    Position currentPos = pathAgent[t];

    for (int agentId2 = 0; agentId2 < globalMemory.agentsCount; agentId2++) {
        if (agentId == agentId2) continue;

        Position* pathOther = &globalMemory.agentPaths[agentId2 * mapSize];
        int otherPathSize = globalMemory.agents[agentId2].sizePath;

        if (t >= otherPathSize - 1) continue;

        Position otherNextPos = pathOther[t + 1];
        Position otherCurrentPos = pathOther[t];

        if (nextPos.x == otherCurrentPos.x && nextPos.y == otherCurrentPos.y &&
            otherNextPos.x == currentPos.x && otherNextPos.y == currentPos.y) {
            conflictAgentId = agentId2;
            conflictFuturePos = { otherNextPos, MOVE_CONSTRATINT | (agentId2 << 2), t };
            return true;
        }

        if (otherNextPos.x == nextPos.x && otherNextPos.y == nextPos.y) {
            conflictAgentId = agentId2;
            conflictFuturePos = { otherNextPos, MOVE_CONSTRATINT | (agentId2 << 2), t };
            return true;
        }
    }
    return false;
}

void lookRemoveOldConstrait(int agentID, const MemoryPointers& globalMemory, MemoryPointers& localMemory) {
    int count = localMemory.numberConstraits[agentID], newCount = 0, mapSize = localMemory.height*localMemory.width;
    if (count == 0) return;
    Constrait* constraints = localMemory.constraits;
    for (int i = 0; i < count; i++){
        Constrait c = constraints[i];
        Position p = c.to;
        int t = c.timeStep, otherID = (c.type >> 2);
        Agent a = globalMemory.agents[otherID];
        if (a.sizePath <= t){
            continue;
        }
        Position otherP = globalMemory.agentPaths[otherID * mapSize + t];
        if (otherP.x != p.x || otherP.y != p.y){
            continue;
        }
        constraints[newCount] = c;
        newCount++;
    }
    localMemory.numberConstraits[agentID] = newCount;
}


bool shouldYield(int agentId, int conflictAgentId, Agent* agents) {
    Agent& a = agents[agentId], & b = agents[conflictAgentId];
    if (a.direction != b.direction) {
        return (a.direction == AGENT_LOADER);
    }
    if (a.sizePath != b.sizePath) {
        return (a.sizePath > b.sizePath);
    }
    return (agentId > conflictAgentId);
}

void processAgentCollisionsGPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, sycl::nd_item<1> item) {
    int agentID = item.get_local_id(0);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        collision_flag(*globalMemory.minSize_numberColision);
    collision_flag.fetch_or(1);
    positionHeap heap = { localMemory.openList, 0 };
    computePathForAgent_DStar(agentID, localMemory, heap);
    Agent a = localMemory.agents[agentID];
    Position start = { a.x, a.y };
    Position goal;
    if (a.direction == AGENT_LOADER) {
        goal = localMemory.loaderPositions[a.loaderCurrent];
    }
    else {
        goal = localMemory.unloaderPositions[a.unloaderCurrent];
    }
    while (true) {
        item.barrier();
        if (collision_flag.load() == 0) { break; }
        localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory, start, goal);
        item.barrier();
        if (agentID == 0) { collision_flag.store(0); }
        int pathSize = localMemory.agents[agentID].sizePath;
        lookRemoveOldConstrait(agentID, globalMemory, localMemory);
        for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
            int conflictAgentId = -1;
            Constrait conflictFuturePos = { { -1, -1 }, -1, -1 };
            if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                int indexTopConstrait = localMemory.numberConstraits[agentID];
                localMemory.constraits[indexTopConstrait] = conflictFuturePos;
                localMemory.numberConstraits[agentID] += 1;
                collision_flag.fetch_or(1);
                int conflictIndex = getTrueIndexGrid(localMemory.width, conflictFuturePos.to.x, conflictFuturePos.to.y);
                updateVertex_DStar(agentID, localMemory, heap, localMemory.cameFrom[conflictIndex], goal);
                computeShortestPath_DStar(agentID, localMemory, heap, start, goal);
                break;
            }
        }
    }
}

void processAgentCollisionsCPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, MyBarrier& item, int agentID) {
    std::atomic<int>& collision_flag = reinterpret_cast<std::atomic<int>&>(*globalMemory.minSize_numberColision);
    collision_flag.fetch_or(1);
    positionHeap heap = { localMemory.openList, 0 };
    computePathForAgent_DStar(agentID, localMemory, heap);
    Agent a = localMemory.agents[agentID];
    Position start = { a.x, a.y };
    Position goal;
    if (a.direction == AGENT_LOADER) {
        goal = localMemory.loaderPositions[a.loaderCurrent];
    }
    else {
        goal = localMemory.unloaderPositions[a.unloaderCurrent];
    }
    while (true) {
        item.barrier();
        if (collision_flag.load() == 0) { break; }
        localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory, start, goal);
        item.barrier();
        if (agentID == 0) { collision_flag.store(0); }
        int pathSize = localMemory.agents[agentID].sizePath;
        lookRemoveOldConstrait(agentID, globalMemory, localMemory);
        for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
            int conflictAgentId = -1;
            Constrait conflictFuturePos = { { -1, -1 }, -1, -1 };
            if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                int indexTopConstrait = localMemory.numberConstraits[agentID];
                localMemory.constraits[indexTopConstrait] = conflictFuturePos;
                localMemory.numberConstraits[agentID] += 1;
                collision_flag.fetch_or(1);
                int conflictIndex = getTrueIndexGrid(localMemory.width, conflictFuturePos.to.x, conflictFuturePos.to.y);
                updateVertex_DStar(agentID, localMemory, heap, localMemory.cameFrom[conflictIndex], goal);
                computeShortestPath_DStar(agentID, localMemory, heap, start, goal);
                break;
            }
        }
    }
}

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory) {
    int collision_flag = 1;
    int agentCount = globalMemory.agentsCount;
    std::vector<MemoryPointers> localMemorys(agentCount);
    std::vector<positionHeap> heaps(agentCount);
    std::vector<Position> starts(agentCount);
    std::vector<Position> goals(agentCount);

    for (int agentID = 0; agentID < agentCount; agentID++){
        fillLocalMemory(globalMemory, agentID, localMemorys[agentID]);
        heaps[agentID] = {localMemorys[agentID].openList, 0};
        computePathForAgent_DStar(agentID, localMemorys[agentCount], heaps[agentID]);
        Agent a = localMemorys[agentID].agents[agentID];
        starts[agentID] = { a.x, a.y };
        if (a.direction == AGENT_LOADER) {
            goals[agentID] = localMemorys[agentID].loaderPositions[a.loaderCurrent];
        }
        else {
            goals[agentID] = localMemorys[agentID].unloaderPositions[a.unloaderCurrent];
        }
    }
    while (true) {
        if (collision_flag == 0) { break; }
        collision_flag = 0;
        for (int agentID = 0; agentID < agentCount; agentID++) {
            localMemorys[agentID].agents[agentID].sizePath = reconstructPath(agentID, localMemorys[agentID], starts[agentID], goals[agentID]);
        }
        for (int agentID = 0; agentID < agentCount; agentID++) {
            MemoryPointers& localMemory = localMemorys[agentID];
            int pathSize = localMemory.agents[agentID].sizePath;
            lookRemoveOldConstrait(agentID, globalMemory, localMemory);
            for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
                int conflictAgentId = -1;
                Constrait conflictFuturePos = { { -1, -1 }, -1, -1 };
                if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                    shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                    int indexTopConstrait = localMemory.numberConstraits[agentID];
                    localMemory.constraits[indexTopConstrait] = conflictFuturePos;
                    localMemory.numberConstraits[agentID] += 1;
                    collision_flag = 1;
                    int conflictIndex = getTrueIndexGrid(localMemory.width, conflictFuturePos.to.x, conflictFuturePos.to.y);
                    updateVertex_DStar(agentID, localMemory, heaps[agentID], localMemory.cameFrom[conflictIndex], goals[agentID]);
                    computeShortestPath_DStar(agentID, localMemory, heaps[agentID], starts[agentID], goals[agentID]);
                    break;
                }
            }
        }
    }
}