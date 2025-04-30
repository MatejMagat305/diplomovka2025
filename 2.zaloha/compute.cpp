// struct
/*
#define AGENT_UNLOADER 1
#define AGENT_LOADER 2
#define AGENT_PARK_LOADER 3
#define AGENT_PARK_UNLOADER 4

#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

// nno map symbol
#define AGENT_SYMBOL 'A'

class MyBarrier {
    std::mutex mutex;
    std::condition_variable cond;
    int count;
    const int origin;  // Počet vlákien, ktoré musia doraziť na bariéru

public:
    explicit MyBarrier(int count) : count(count), origin(count) {}

    void barrier() {
        std::unique_lock<std::mutex> lock(mutex);  // Použi unique_lock pre wait()

        if (--count == 0) {
            count = origin;  // Resetujeme count pre ďalšie použitie
            cond.notify_all();  // Odomkneme všetky čakajúce vlákna
        }
        else {
            cond.wait(lock, [this]() { return count == origin; });  // Čakáme na reset countu
        }
    }
};
struct Agent {
    int sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction, mustWait = 0;
    Position position, goal;
};

struct Position {
    int x, y;

    bool operator<(const Position& other) const;
    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;
};

#define MOVE_CONSTRATNT 1
#define BLOCK_CONSTRATNT 2
struct Constraint {
    Position position;
    int type, timeStep;
};

struct HeapPositionNode {
    Position pos;
    int priority;
};

struct positionHeap {
    HeapPositionNode* heap;
    int size;
};

struct MemoryPointers {
    int width, height, loadersCount, unloadersCount, agentsCount;
    char* grid = nullptr;
    Position* loaderPositions = nullptr;
    Position* unloaderPositions = nullptr;
    Agent* agents = nullptr;

    int* minSize_numberColision = nullptr;
    Position* agentPaths = nullptr;

    int* gCost = nullptr;
    int* fCost = nullptr;
    Position* cameFrom = nullptr;
    bool* visited = nullptr;

    HeapPositionNode* openList = nullptr;
    Constraint* constraints = nullptr;
    int* numberConstraints = nullptr;
};

// helper function

void push(positionHeap& myHeap, Position p, int priority) {
    int i = myHeap.size;
    myHeap.heap[i].pos = p;
    myHeap.heap[i].priority = priority;
    myHeap.size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (myHeap.heap[parent].priority <= myHeap.heap[i].priority)
            break;
        HeapPositionNode temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[parent];
        myHeap.heap[parent] = temp;
        i = parent;
    }
}

Position pop(positionHeap& myHeap) {
    if (myHeap.size == 0) {
        return { -1, -1 };
    }
    Position root = myHeap.heap[0].pos;
    myHeap.size--;
    myHeap.heap[0] = myHeap.heap[myHeap.size];

    int i = 0;
    while (2 * i + 1 < myHeap.size) {
        int left = 2 * i + 1, right = 2 * i + 2;
        int smallest = left;
        if (right < myHeap.size && myHeap.heap[right].priority < myHeap.heap[left].priority)
            smallest = right;
        if (myHeap.heap[i].priority <= myHeap.heap[smallest].priority)
            break;
        HeapPositionNode temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[smallest];
        myHeap.heap[smallest] = temp;
        i = smallest;
    }
    return root;
}

bool empty(positionHeap& myHeap) {
    return myHeap.size == 0;
}


inline int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}
#include "fill_init_convert.h"
#include "sycl/sycl.hpp"

int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}

int myAbs(int input) {
    int mask = input >> 31;  // Vytvorí -1 pre záporné čísla, 0 pre kladné
    return (input + mask) ^ mask;
}

int reconstructPath(int agentID, MemoryPointers& localMemory) {
    const int width = localMemory.width;
    const int height = localMemory.height;
    const int mapSize = width * height;
    int pathLength = 0;
    Agent& a = localMemory.agents[agentID];
    Position start = a.position;
    Position current = a.goal;
    localMemory.agentPaths[pathLength++] = current;
    for (int i = 0; i < mapSize; i++) {
        int currentIndex = getTrueIndexGrid(width, current.x, current.y);
        if (currentIndex < 0 || currentIndex >= mapSize) {
            pathLength = 0;
            break;
        }
        Position next = localMemory.cameFrom[currentIndex];
        if (next.x == -1 || next.y == -1) {
            pathLength = 0;
            break;
        }
        current = next;
        localMemory.agentPaths[pathLength++] = current;
        if (current.x == start.x && current.y == start.y) {
            break;
        }
    }
    for (int i = 0; i < pathLength / 2; i++) {
        Position temp = localMemory.agentPaths[i];
        localMemory.agentPaths[i] = localMemory.agentPaths[pathLength - i - 1];
        localMemory.agentPaths[pathLength - i - 1] = temp;
    }
    return pathLength;
}


void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory) {
    localMemory.width = globalMemory.width;
    localMemory.height = globalMemory.height;
    localMemory.agentsCount = globalMemory.agentsCount;
    localMemory.loadersCount = globalMemory.loadersCount;
    localMemory.unloadersCount = globalMemory.unloadersCount;

    const int mapSize = localMemory.width * localMemory.height;
    int offset = mapSize * agentId;
    localMemory.grid = globalMemory.grid;
    localMemory.loaderPositions = globalMemory.loaderPositions;
    localMemory.unloaderPositions = globalMemory.unloaderPositions;
    localMemory.agents = globalMemory.agents;
    localMemory.minSize_numberColision_error = globalMemory.minSize_numberColision_error;
    localMemory.agentPaths = &(globalMemory.agentPaths[offset]);
    localMemory.gCost = &(globalMemory.gCost[offset]);
    localMemory.fCost = &(globalMemory.fCost[offset]);
    localMemory.cameFrom = &(globalMemory.cameFrom[offset]);
    localMemory.visited = &(globalMemory.visited[offset]);
    localMemory.openList = &(globalMemory.openList[offset]);
    localMemory.constraints = &(globalMemory.constraints[offset / 4]);
    localMemory.numberConstraints = globalMemory.numberConstraints;
}
void initializeCostsAndHeap(MemoryPointers& localMemory, positionHeap& myHeap, Position start) {
    const int width = localMemory.width;
    const int height = localMemory.height;
    const int mapSize = width * height;
    for (int i = 0; i < mapSize; i++) {
        localMemory.visited[i] = false;
        localMemory.gCost[i] = INF;
        localMemory.fCost[i] = INF;
    }
    int index = getTrueIndexGrid(width, start.x, start.y);
    localMemory.gCost[index] = 0;
    localMemory.fCost[index] = 0;
    push(myHeap, start, localMemory.fCost[index]);
}


std::vector<std::vector<Position>> getVector(Map* m) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    size_t offset = CPUMemory.width * CPUMemory.height;
    std::vector<std::vector<Position>> result(CPUMemory.agentsCount);

    Position* ptr = CPUMemory.agentPaths;
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        size_t pathSize = CPUMemory.agents[i].sizePath;
        result[i].assign(ptr, ptr + pathSize);  // Skopírujeme dáta do vektora
        ptr += offset;  // Posun na ďalšieho agenta
    }
    return result;
}


void pushVector(const std::vector<std::vector<Position>>& vec2D, Map* m) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    size_t offset = CPUMemory.width * CPUMemory.height;
    Position* ptr = CPUMemory.agentPaths;
    int minSize = vec2D[0].size();
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        const std::vector<Position>& row = vec2D[i];
        std::copy(row.begin(), row.end(), ptr);
        ptr += offset;
        int agentSizePath = row.size();
        CPUMemory.agents[i].sizePath = agentSizePath;
        if (agentSizePath < minSize) {
            minSize = agentSizePath;
        }
    }
    *CPUMemory.minSize_numberColision_error = minSize;
}

int ManhattanHeuristic(const Position& a, const Position& b) {
    return myAbs(a.x - b.x) + myAbs(a.y - b.y);
}
// a* algoritmus

int reconstructPath(int agentID, MemoryPointers& localMemory) {
    Agent& a = localMemory.agents[agentID];
    Position start = a.position;
    Position goal = a.goal;
    const int width = localMemory.width;
    const int mapSize = width * localMemory.height;
    Position current = goal;
    int pathLength = 0;

    while (!isSamePosition(start, current)) {
        if (current.x == -1 || current.y == -1) {
            return 0;
        }
        int index = getTrueIndexGrid(width, current.x, current.y);
        if ((index >= 0 || index >= mapSize)) {
            return 0;
        }
        localMemory.agentPaths[pathLength] = current;
        pathLength++;
        current = localMemory.cameFrom[index];
    }
    localMemory.agentPaths[pathLength] = start;
    pathLength++;
    for (int i = 0; i < pathLength / 2; i++) {
        Position temp = localMemory.agentPaths[i];
        localMemory.agentPaths[i] = localMemory.agentPaths[pathLength - i - 1];
        localMemory.agentPaths[pathLength - i - 1] = temp;
    }
    return pathLength;
}

bool isBlocked(int agentId, Position next, int newG, MemoryPointers& localMemory) {
    int numConstraints = localMemory.numberConstraints[agentId];
    for (int i = 0; i < numConstraints; i++) {
        Constraint c = localMemory.constraints[i];
        if (c.position.x == next.x && c.position.y == next.y && c.timeStep == newG) {
            return true;
        }
    }
    return false;
}

bool runAStarGetIsReached(MemoryPointers& localMemory, positionHeap& myHeap, Position start, Position goal, int agentId) {
    const int width = localMemory.width;
    const int height = localMemory.height;

    while (!empty(myHeap)) {
        Position current = pop(myHeap);
        if (isSamePosition(current, goal)) {
            return true;
        }
        int index = getTrueIndexGrid(width, current.x, current.y);

        if (localMemory.visited[index]) {
            continue;
        }
        localMemory.visited[index] = true;

        Position neighbors[4] = {
            {current.x + 1, current.y}, {current.x - 1, current.y},
            {current.x, current.y + 1}, {current.x, current.y - 1}
        };

        for (int j = 0; j < 4; j++) {
            Position next = neighbors[j];
            int nextIndex = getTrueIndexGrid(width, next.x, next.y);

            if (next.x >= 0 && next.x < width && next.y >= 0 && next.y < height &&
                localMemory.grid[nextIndex] != OBSTACLE_SYMBOL &&
                !localMemory.visited[nextIndex]) {
                int newG = localMemory.gCost[index] + 1;
                if (isBlocked(agentId, next, newG, localMemory)) {
                    continue;
                }
                if (newG < localMemory.gCost[nextIndex]) {
                    localMemory.gCost[nextIndex] = newG;
                    int newFCost = newG + myAbs(goal.x - next.x) + myAbs(goal.y - next.y);
                    localMemory.fCost[nextIndex] = newFCost;
                    localMemory.cameFrom[nextIndex] = current;
                    push(myHeap, next, newFCost);
                }
            }
        }
    }
    return false;
}


bool astar_algorithm_get_sussces(int agentId, MemoryPointers& localMemory) {
    Agent a = localMemory.agents[agentId];
    Position start = a.position;
    Position goal = a.goal;
    positionHeap myHeap{ localMemory.openList, 0 };
    initializeCostsAndHeap(localMemory, myHeap, start);
    return runAStarGetIsReached(localMemory, myHeap, start, goal, agentId);
}
bool isSamePosition(Position& a, Position& b) {
    return bool(a.x == b.x && a.y == b.y);
}

// solve conflicts
#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <thread> 
#include "fill_init_convert.h"
#include "a_star_algo.h"

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

bool checkCollision(int agentId, int& conflictAgentId, int t, Constraint& conflictFuturePos, const MemoryPointers& globalMemory) {
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
            conflictFuturePos = {};
            conflictFuturePos.position = otherNextPos;
            conflictFuturePos.type = MOVE_CONSTRATNT | (agentId2 << 2);
            conflictFuturePos.timeStep = t;
            return true;
        }

        if (otherNextPos.x == nextPos.x && otherNextPos.y == nextPos.y) {
            conflictAgentId = agentId2;
            conflictFuturePos = {};
            conflictFuturePos.position = otherNextPos;
            conflictFuturePos.type = MOVE_CONSTRATNT | (agentId2 << 2);
            conflictFuturePos.timeStep = t;
            return true;
        }
    }
    return false;
}

bool findFreeBlockPark(int agentId, const MemoryPointers& globalMemory, MemoryPointers& localMemory) {
    Agent& a = localMemory.agents[agentId];
    const int width = localMemory.width;
    const int height = localMemory.height;

    int numberConstraints = localMemory.numberConstraints[agentId];
    Constraint& c = localMemory.constraints[numberConstraints - 1];
    int type = c.type;
    int OtherId = ((type & 12) >> 2);
    c.type = BLOCK_CONSTRATNT | (OtherId << 2);

    Constraint constraint = c;
    if (a.mustWait != 0) {
        for (int i = numberConstraints - 2; i > -1; i--) {
            if ((localMemory.constraints[i].type & 3) == MOVE_CONSTRATNT) {
                constraint = localMemory.constraints[i + 1];
                break;
            }
        }
    }
    a.mustWait = 1;

    OtherId = ((constraint.type & 12) >> 2);
    int agentOtherSizePath = globalMemory.agents[OtherId].sizePath;
    Position* otherPath = &globalMemory.agentPaths[OtherId * width * height];
    for (int i = constraint.timeStep; i < agentOtherSizePath; i++) {
        Position currentPos = otherPath[i];
        Position neighbors[4] = {
            { currentPos.x + 1, currentPos.y },
            { currentPos.x - 1, currentPos.y },
            { currentPos.x, currentPos.y + 1 },
            { currentPos.x, currentPos.y - 1 }
        };

        for (int i = 0; i < 4; i++) {
            Position next = neighbors[i];
            if (next.x < 0 || next.y < 0 || next.x >= width || next.y >= height) {
                continue;
            }
            int index = getTrueIndexGrid(width, next.x, next.y);
            if (localMemory.grid[index] == OBSTACLE_SYMBOL) {
                continue;
            }
            bool blocked = false;
            for (int j = 0; j < localMemory.numberConstraints[agentId]; j++) {
                Constraint c = localMemory.constraints[j];
                if (c.position.x == next.x && c.position.y == next.y) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                continue;
            }
            if (astar_algorithm_get_sussces(agentId, localMemory)) {
                a.goal = next;
                return true;
            }
        }
    }
    return false;
}


void lookRemoveOldConstrait(int agentID, const MemoryPointers& globalMemory, MemoryPointers& localMemory) {
    int count = localMemory.numberConstraints[agentID], newCount = 0, mapSize = localMemory.height * localMemory.width;
    if (count == 0) return;
    Constraint* constraints = localMemory.constraints;
    for (int i = 0; i < count; i++) {
        Constraint c = constraints[i];
        Position p = c.position;
        int t = c.timeStep, otherID = (c.type >> 2);
        Agent a = globalMemory.agents[otherID];
        if (a.sizePath <= t) {
            continue;
        }
        Position otherP = globalMemory.agentPaths[otherID * mapSize + t];
        if (otherP.x != p.x || otherP.y != p.y) {
            continue;
        }
        constraints[newCount] = c;
        newCount++;
    }
    localMemory.numberConstraints[agentID] = newCount;
}


bool shouldYield(int agentId, int conflictAgentId, Agent* agents) {
    Agent& a = agents[agentId], & b = agents[conflictAgentId];
    if (a.direction == AGENT_PARK_LOADER || a.direction == AGENT_PARK_UNLOADER) {
        return true;
    }
    if (b.direction == AGENT_PARK_LOADER || b.direction == AGENT_PARK_UNLOADER) {
        return false;
    }
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
        collision_flag(*globalMemory.minSize_numberColision_error);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        err_flag(globalMemory.minSize_numberColision_error[1]);
    localMemory.numberConstraints[agentID] = 0;
    collision_flag.store(1);
    err_flag.store(0);
    bool private_reconstruct_flag = true;
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
        err_flag.store(agentID + 1);
    }
    while (true) {
        item.barrier();
        if (collision_flag.load() == 0 || err_flag.load() != 0) { break; }
        if (private_reconstruct_flag) {
            localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory);
            private_reconstruct_flag = false;
        }
        item.barrier();
        if (agentID == 0) { collision_flag.store(0); }
        int pathSize = localMemory.agents[agentID].sizePath;
        lookRemoveOldConstrait(agentID, globalMemory, localMemory);
        for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
            int conflictAgentId = -1;
            Constraint conflictFuturePos = { { -1, -1 }, -1, -1 };
            if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                int indexTopConstrait = localMemory.numberConstraints[agentID];
                localMemory.constraints[indexTopConstrait] = conflictFuturePos;
                localMemory.numberConstraints[agentID] += 1;
                collision_flag.store(1);
                if (!astar_algorithm_get_sussces(agentID, localMemory)) {
                    if (findFreeBlockPark(agentID, globalMemory, localMemory)) {
                        err_flag.store(agentID + 1);
                        break;
                    }
                }
                private_reconstruct_flag = true;
                break;
            }
        }
    }
}

void processAgentCollisionsCPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, MyBarrier& item, int agentID) {
    std::atomic<int>& collision_flag = reinterpret_cast<std::atomic<int>&>(*globalMemory.minSize_numberColision_error);
    std::atomic<int>& err_flag = reinterpret_cast<std::atomic<int>&>(globalMemory.minSize_numberColision_error[1]);

    localMemory.numberConstraints[agentID] = 0;
    collision_flag.store(1);
    collision_flag.store(0);
    bool private_reconstruct_flag = true;
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
        err_flag.store(agentID + 1);
    }
    while (true) {
        item.barrier();
        if (collision_flag.load() == 0 || err_flag.load() != 0) { break; }
        if (private_reconstruct_flag) {
            localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory);
            private_reconstruct_flag = false;
        }
        item.barrier();
        if (agentID == 0) { collision_flag.store(0); }
        int pathSize = localMemory.agents[agentID].sizePath;
        lookRemoveOldConstrait(agentID, globalMemory, localMemory);
        for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
            int conflictAgentId = -1;
            Constraint conflictFuturePos = { { -1, -1 }, -1, -1 };
            if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                int indexTopConstrait = localMemory.numberConstraints[agentID];
                localMemory.constraints[indexTopConstrait] = conflictFuturePos;
                localMemory.numberConstraints[agentID] += 1;
                collision_flag.store(1);
                if (!astar_algorithm_get_sussces(agentID, localMemory)) {
                    if (findFreeBlockPark(agentID, globalMemory, localMemory)) {
                        err_flag.store(agentID + 1);
                        break;
                    }
                }
                private_reconstruct_flag = true;
                break;
            }
        }
    }
}

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory) {
    int collision_flag = 1;
    int agentCount = globalMemory.agentsCount;
    std::vector<MemoryPointers> localMemorys(agentCount);
    std::vector<bool> private_reconstruct_flags(agentCount);

    for (int agentID = 0; agentID < agentCount; agentID++) {
        MemoryPointers& localMemory = localMemorys[agentID];
        fillLocalMemory(globalMemory, agentID, localMemory);
        localMemory.numberConstraints[agentID] = 0;
        if (!astar_algorithm_get_sussces(agentID, localMemory)) {
            globalMemory.minSize_numberColision_error[1] = agentID + 1;
        }
        private_reconstruct_flags[agentID] = true;
    }
    while (collision_flag != 0 && globalMemory.minSize_numberColision_error[1] == 0) {
        collision_flag = 0;
        for (int agentID = 0; agentID < agentCount; agentID++) {
            if (private_reconstruct_flags[agentID]) {
                localMemorys[agentID].agents[agentID].sizePath = reconstructPath(agentID, localMemorys[agentID]);
                private_reconstruct_flags[agentID] = false;
            }
        }
        for (int agentID = 0; agentID < agentCount; agentID++) {
            MemoryPointers& localMemory = localMemorys[agentID];
            int pathSize = localMemory.agents[agentID].sizePath;
            lookRemoveOldConstrait(agentID, globalMemory, localMemory);
            for (int timeStep = 0; timeStep < pathSize - 1; timeStep++) {
                int conflictAgentId = -1;
                Constraint conflictFuturePos = { { -1, -1 }, -1, -1 };
                if (checkCollision(agentID, conflictAgentId, timeStep, conflictFuturePos, globalMemory) &&
                    shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                    int indexTopConstrait = localMemory.numberConstraints[agentID];
                    localMemory.constraints[indexTopConstrait] = conflictFuturePos;
                    localMemory.numberConstraints[agentID] += 1;
                    collision_flag = 1;
                    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
                        if (findFreeBlockPark(agentID, globalMemory, localMemory)) {
                            globalMemory.minSize_numberColision_error[1] = agentID + 1;
                            break;
                        }
                    }
                    private_reconstruct_flags[agentID] = true;
                    break;
                }
            }
        }
    }
    writeMinimalPath(globalMemory);
}
// sycl
MemoryPointers GPUMemory{};
sycl::queue& getQueue() {
    static sycl::queue q{ sycl::default_selector_v };
    return q;
}

std::string initializeSYCLMemory(Map* m) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    size_t mapSize = CPUMemory.width * CPUMemory.height;
    size_t stackSize = CPUMemory.agentsCount * mapSize;

    long long totalAlloc = 0;
    totalAlloc += sizeof(GPUMemory) * 2; // data structuree
    totalAlloc += sizeof(int)*2; // minSize
    totalAlloc += mapSize * sizeof(char); // grid
    totalAlloc += CPUMemory.agentsCount * sizeof(Agent); // agents
    totalAlloc += CPUMemory.loadersCount * sizeof(Position); // loaderPosition
    totalAlloc += CPUMemory.unloadersCount * sizeof(Position); // unloaderPosition
    totalAlloc += stackSize * sizeof(Position); // paths
    totalAlloc += CPUMemory.agentsCount * sizeof(int); // contraitsSizes
    totalAlloc += stackSize * sizeof(int); // gCost
    totalAlloc += stackSize * sizeof(int); // rhs
    totalAlloc += stackSize * sizeof(int); // fCost
    totalAlloc += stackSize * sizeof(Position); // cameFrom
    totalAlloc += stackSize * sizeof(bool); // visited
    totalAlloc += stackSize * sizeof(HeapPositionNode); // openList
    totalAlloc += stackSize * sizeof(Constraint) / 4; // constrait
    totalAlloc += CPUMemory.agentsCount * sizeof(int); // numberConstrait
    long long totalGlobalMem = 4LL * 1024 * 1024 * 1024; //q->get_device().get_info<sycl::info::device::global_mem_size>(); // 4 GB on cpu too
    if (totalAlloc >= ((totalGlobalMem * 3) / 4)) {
        std::stringstream ss;
        ss << "you have not enough device memory" << totalAlloc << "/" << ((totalGlobalMem * 3) / 4);
        return ss.str();
    }
    try {
        sycl::queue& q = getQueue();
        GPUMemory.minSize_numberColision_error = sycl::malloc_device<int>(2, q);
        GPUMemory.grid = sycl::malloc_device<char>(mapSize, q);
        GPUMemory.agents = sycl::malloc_device<Agent>(CPUMemory.agentsCount, q);
        GPUMemory.loaderPositions = sycl::malloc_device<Position>(CPUMemory.loadersCount, q);
        GPUMemory.unloaderPositions = sycl::malloc_device<Position>(CPUMemory.unloadersCount, q);
        GPUMemory.agentPaths = sycl::malloc_device<Position>(stackSize, q);
        GPUMemory.gCost = sycl::malloc_device<int>(stackSize, q);
        GPUMemory.fCost = sycl::malloc_device<int>(stackSize, q);
        GPUMemory.cameFrom = sycl::malloc_device<Position>(stackSize, q);
        GPUMemory.visited = sycl::malloc_device<bool>(stackSize, q);
        GPUMemory.openList = sycl::malloc_device<HeapPositionNode>(stackSize, q);
        GPUMemory.constraints = sycl::malloc_device<Constraint>(stackSize / 4, q);
        GPUMemory.numberConstraints = sycl::malloc_device<int>(CPUMemory.agentsCount, q);
        sycl::event e1 = q.memcpy(GPUMemory.grid, CPUMemory.grid, mapSize * sizeof(char));
        sycl::event e2 = q.memcpy(GPUMemory.loaderPositions, CPUMemory.loaderPositions, CPUMemory.loadersCount * sizeof(Position));
        sycl::event e3 = q.memcpy(GPUMemory.unloaderPositions, CPUMemory.unloaderPositions, CPUMemory.unloadersCount * sizeof(Position));
        GPUMemory.width = CPUMemory.width;
        GPUMemory.height = CPUMemory.height;
        GPUMemory.loadersCount = CPUMemory.loadersCount;
        GPUMemory.unloadersCount = CPUMemory.unloadersCount;
        GPUMemory.agentsCount = CPUMemory.agentsCount;
        sycl::event::wait_and_throw({ e1, e2, e3 });
    }
    catch (sycl::exception& e) {
        std::stringstream ss;
        ss << e.what();
        return ss.str();
    }
    return "";
}

void synchronizeCPUFromGPU(Map* m) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    sycl::queue& q = getQueue();
    sycl::event e1 = q.memcpy(CPUMemory.agents, GPUMemory.agents, CPUMemory.agentsCount * sizeof(Agent));
    sycl::event e3 = q.memcpy(CPUMemory.agentPaths, GPUMemory.agentPaths, CPUMemory.agentsCount * CPUMemory.width * CPUMemory.height * sizeof(Position));
    sycl::event e4 = q.memcpy(CPUMemory.minSize_numberColision_error, GPUMemory.minSize_numberColision_error, sizeof(int));
    sycl::event::wait_and_throw({ e1, e3, e4 });
}

void synchronizeGPUFromCPU(Map* m) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    sycl::queue& q = getQueue();
    sycl::event e1 = q.memcpy(GPUMemory.agents, CPUMemory.agents, CPUMemory.agentsCount * sizeof(Agent));
    sycl::event e3 = q.memcpy(GPUMemory.agentPaths, CPUMemory.agentPaths, CPUMemory.agentsCount * CPUMemory.width * CPUMemory.height * sizeof(Position));
    sycl::event e4 = q.memcpy(GPUMemory.minSize_numberColision_error, CPUMemory.minSize_numberColision_error, sizeof(int));
    sycl::event::wait_and_throw({ e1, e3, e4 });
}

inline void freePtr(void** ptr) {
    if (ptr != nullptr && *ptr != nullptr) {
        sycl::free(*ptr, getQueue());
        *ptr = nullptr;
    }
}

void deleteGPUMem() {
    sycl::queue& q = getQueue();
    q.wait();
    freePtr((void**)(&(GPUMemory.minSize_numberColision_error)));
    freePtr((void**)(&(GPUMemory.unloaderPositions)));
    freePtr((void**)(&(GPUMemory.loaderPositions)));
    freePtr((void**)(&(GPUMemory.grid)));
    freePtr((void**)(&(GPUMemory.agents)));

    freePtr((void**)(&(GPUMemory.visited)));
    freePtr((void**)(&(GPUMemory.cameFrom)));
    freePtr((void**)(&(GPUMemory.fCost)));
    freePtr((void**)(&(GPUMemory.gCost)));
    freePtr((void**)(&(GPUMemory.openList)));
    freePtr((void**)(&(GPUMemory.agentPaths)));
    freePtr((void**)(&(GPUMemory.constraints)));
    freePtr((void**)(&(GPUMemory.numberConstraints)));
    q.wait();
}

void SYCL_AStar(Map* m) {
    sycl::queue& q = getQueue();
    int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
    // pohyb o minimálnu cestu 
    auto moveTO = q.submit([&](sycl::handler& h) {
        MemoryPointers globalMemory = GPUMemory;
        h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
            const int id = idx[0];
            MemoryPointers localMemory;
            fillLocalMemory(globalMemory, id, localMemory);
            moveAgentForIndex(id, localMemory);
            });
        });
    // cesty a kolízie
    auto collisionEvent = q.submit([&](sycl::handler& h) {
        h.depends_on(moveTO);
        MemoryPointers globalMemory = GPUMemory;
        sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)));
        h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
            const int id = item.get_global_id(0);
            MemoryPointers localMemory;
            fillLocalMemory(globalMemory, id, localMemory);
            astar_algorithm_get_sussces(id, localMemory);
            processAgentCollisionsGPU(globalMemory, localMemory, item);
            });
        });

    sycl::event minEvent = q.submit([&](sycl::handler& h) {
        h.depends_on(collisionEvent);
        MemoryPointers globalMemory = GPUMemory;
        h.single_task([=]() {
            writeMinimalPath(globalMemory);
            });
        });
    minEvent.wait_and_throw();
}

//====================== CPU HIGH LEVEL ===============================

std::vector<Position> ComputeASTAR(Map* m, int agentID, const std::vector<std::vector<Constraint>>& constraints) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    Agent& a = CPUMemory.agents[agentID];
    Position start = a.position, goal = a.goal;
    int width = CPUMemory.width, height = CPUMemory.height;

    std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, CompareNodes> openSet;
    std::unordered_map<Position, std::shared_ptr<Node>> cameFrom;
    std::unordered_map<Position, int> costSoFar;

    openSet.emplace(std::make_shared<Node>(start, 0, ManhattanHeuristic(start, goal)));
    costSoFar[start] = 0;

    // Rýchle vyhľadávanie obmedzení
    std::unordered_map<Position, std::unordered_set<int>> constraintSet;
    if (!constraints.empty()) {
        for (const auto& constraint : constraints[agentID]) {
            constraintSet[constraint.position].insert(constraint.timeStep);
        }
    }

    while (!openSet.empty()) {
        auto current = openSet.top();
        openSet.pop();

        if (isSamePosition(current->pos, goal)) {
            std::vector<Position> path;
            while (current) {
                path.push_back(current->pos);
                current = current->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        std::vector<Position> neighbors = {
            {current->pos.x + 1, current->pos.y},
            {current->pos.x - 1, current->pos.y},
            {current->pos.x, current->pos.y + 1},
            {current->pos.x, current->pos.y - 1}
        };

        for (auto& next : neighbors) {
            if (next.x < 0 || next.y < 0 || next.x >= width || next.y >= height) continue;
            if (CPUMemory.grid[getTrueIndexGrid(width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;

            int newCost = costSoFar[current->pos] + 1;
            int timeStep = newCost; // Približné časovanie krokov

            // Kontrola časových obmedzení
            if (constraintSet.count(next) && constraintSet[next].count(timeStep)) continue;

            if (!costSoFar.count(next) || newCost < costSoFar[next]) {
                costSoFar[next] = newCost;
                auto newNode = std::make_shared<Node>(next, newCost, ManhattanHeuristic(next, goal), current);
                cameFrom[next] = newNode;
                openSet.push(newNode);
            }
        }
    }
    return {}; // Nebola nájdená žiadna cesta
}

struct PositionOwner {
    Position pos1, pos2;
    int agentID1, agentID2, step;
    PositionOwner(Position p1, Position p2, int id1, int id2, int step0) : pos1(p1), pos2(p2), agentID1(id1), agentID2(id2), step(step0) {}
    bool operator==(const PositionOwner& other) const {
        return pos2.x == other.pos2.x && pos2.y == other.pos2.y
            && pos1.x == other.pos1.x && pos1.y == other.pos1.y
            && agentID1 == other.agentID1 && agentID2 == other.agentID2;
    }
    bool operator!=(const PositionOwner& other) const {
        return !(*this == other);
    }
};

struct PositionOwnerHash {
    std::size_t operator()(const PositionOwner& p) const {
        std::size_t seed = 0;
        seed ^= std::hash<int>{}(p.agentID1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.agentID2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos1.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos1.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos2.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.pos2.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct CTNode {
    std::vector<std::vector<Position>> paths;
    std::vector<std::vector<Constraint>> constraints;
    std::unordered_set<PositionOwner, PositionOwnerHash> conflicts;

    bool operator>(const CTNode& other) const {
        return (conflicts.size() > other.conflicts.size());
    }
};

struct Node {
    Position pos;
    int g, h;
    std::shared_ptr<Node> parent;

    Node(Position pos, int g, int h, std::shared_ptr<Node> parent = nullptr)
        : pos(pos), g(g), h(h), parent(parent) {
    }

    int f() const { return g + h; }
};

// Custom comparator for priority_queue (min-heap)
struct CompareNodes {
    bool operator()(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) const {
        return a->f() > b->f(); // Lower f-value has higher priority
    }
};
void reDetectConflicts(CTNode& node, const std::vector<int>& changedAgentIndices) {
    for (int i : changedAgentIndices) {
        for (size_t j = 0; j < node.paths.size(); j++) {
            if (i == j) continue;
            size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
            for (size_t k = 0; k < minS; k++) {
                PositionOwner tupple = PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k);
                if (isSamePosition(node.paths[i][k], node.paths[j][k])) {
                    node.conflicts.insert(tupple);
                    break;
                }
                if (k > 0
                    && isSamePosition(node.paths[i][k], node.paths[j][k - 1])
                    && isSamePosition(node.paths[j][k], node.paths[i][k - 1])) {
                    node.conflicts.insert(tupple);
                    break;
                }
                node.conflicts.erase(tupple);
            }
        }
    }
}

void detectConflicts(CTNode& node) {
    node.conflicts.clear();
    for (size_t i = 0; i < node.paths.size(); i++) {
        for (size_t j = i + 1; j < node.paths.size(); j++) {
            size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
            for (size_t k = 0; k < minS; k++) {
                if (isSamePosition(node.paths[i][k], node.paths[j][k])) {
                    node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k));
                    break;
                }
                if (k > 0 && isSamePosition(node.paths[i][k], node.paths[j][k - 1]) && isSamePosition(node.paths[j][k], node.paths[i][k - 1])) {
                    node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k));
                    break;
                }
            }
        }
    }
}

std::vector<Position> findFreeBlockPark(Map* m, int ID, const CTNode& node, int coliderID) {
    MemoryPointers& CPUMemory = m->CPUMemory;
    int width = CPUMemory.width, height = CPUMemory.height;
    PositionOwner conflictOwner = *node.conflicts.end();
    std::vector<Position> path = node.paths[coliderID];
    int i = conflictOwner.step, maxSize = node.paths[coliderID].size();

    for (; i < maxSize; i++) {
        Position& current = path[i];
        Position& prew = path[i];
        if (i > 0) {
            prew = path[i - 1];
        }
        std::vector<Position> neighbors = {
            {current.x + 1, current.y},
            {current.x - 1, current.y},
            {current.x, current.y + 1},
            {current.x, current.y - 1}
        };
        for (auto& next : neighbors) {
            if (next.x < 0 || next.y < 0 || next.x >= width || next.y >= height) continue;
            if (CPUMemory.grid[getTrueIndexGrid(width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;
            if (isSamePosition(next, conflictOwner.pos1) || isSamePosition(next, conflictOwner.pos2)) continue;
            if (isSamePosition(next, prew)) continue;
            bool blocked = false;
            for (int i = 0; i < node.constraints[ID].size(); i++) {
                Constraint c = node.constraints[ID][i];
                if (c.position.x == next.x && c.position.y == next.y) {
                    blocked = true;
                }
            }
            if (blocked) continue;
            auto path = ComputeASTAR(m, ID, node.constraints);
            CPUMemory.agents[ID].goal = next;
            if (!path.empty()) {
                return path;
            }
        }
    }
    return {};

}

std::vector<std::vector<Position>> resolveConflictsCBS(Map* m, CTNode& root) {
    std::priority_queue<CTNode, std::vector<CTNode>, std::greater<CTNode>> openSet;
    detectConflicts(root);
    openSet.push(root);
    while (!openSet.empty()) {
        CTNode current;
        current = openSet.top();
        openSet.pop();
        if (current.conflicts.empty()) {
            return current.paths;
        }
        for (auto& conflictOwner : current.conflicts) {
            Position conflictPos1 = conflictOwner.pos1, conflictPos2 = conflictOwner.pos2;
            int owner1 = conflictOwner.agentID1, owner2 = conflictOwner.agentID2;
            CTNode newNode1 = current;
            CTNode newNode2 = current;
            Constraint c1, c2;
            c1.position = conflictPos1;
            c2.position = conflictPos2;
            newNode1.constraints[owner1].push_back(c1);
            newNode2.constraints[owner2].push_back(c2);
            auto path1 = ComputeASTAR(m, owner1, newNode1.constraints);
            auto path2 = ComputeASTAR(m, owner2, newNode2.constraints);
            if (path1.empty()) {
                path1 = findFreeBlockPark(m, owner1, newNode1, owner2);
            }
            if (path2.empty()) {
                path2 = findFreeBlockPark(m, owner2, newNode2, owner1);
            }
            if (!path1.empty()) {
                newNode1.paths[owner1] = path1;
                reDetectConflicts(newNode1, { owner1 });
                openSet.push(newNode1);
            }
            if (!path2.empty()) {
                newNode2.paths[owner2] = path2;
                reDetectConflicts(newNode2, { owner2 });
                openSet.push(newNode2);
            }
        }
    }
    return {};
}
Info computeHYBRID(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    sycl::queue& q = getQueue();
    int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
    // pohyb o minimálnu cestu 
    auto moveTO = q.submit([&](sycl::handler& h) {
        MemoryPointers globalMemory = GPUMemory;
        h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
            const int id = idx[0];
            MemoryPointers localMemory;
            fillLocalMemory(globalMemory, id, localMemory);
            moveAgentForIndex(id, localMemory);
            });
        });
    // cesty
    auto foundPathsAgent = q.submit([&](sycl::handler& h) {
        h.depends_on(moveTO);
        MemoryPointers globalMemory = GPUMemory;
        h.parallel_for(sycl::range<1>(std::min(workSize, GPUMemory.agentsCount)), [=](sycl::id<1> idx) {
            const int id = idx[0];
            MemoryPointers localMemory;
            fillLocalMemory(globalMemory, id, localMemory);
            astar_algorithm_get_sussces(id, localMemory);
            localMemory.agents[id].sizePath = reconstructPath(id, localMemory);
            });
        });
    // kolízie
    synchronizeCPUFromGPU(m);
    CTNode root;
    root.paths.resize(GPUMemory.agentsCount);
    root.constraints.resize(GPUMemory.agentsCount);
    root.paths = getVector(m);
    auto solution = resolveConflictsCBS(m, root);
    auto end_time = std::chrono::high_resolution_clock::now();
    pushVector(solution, m);
    auto copy_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
    result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
    return result;
}

Info computeSYCL(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    SYCL_AStar(m);
    auto end_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
    return result;
}


void parallelAStarPrimitive(Map* m) {
    std::vector<std::thread> threads;
    MemoryPointers& CPUMemory = m->CPUMemory;
    // pohyb o minimálnu cestu 
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        threads.emplace_back([&, i]() {
            MemoryPointers local;
            fillLocalMemory(CPUMemory, i, local);
            moveAgentForIndex(i, local);
            });
    }
    for (auto& t : threads) {
        t.join();
    }

    threads.clear();
    MyBarrier b(CPUMemory.agentsCount);

    // cesty a spracovanie kolízií v paralelných vláknach
    for (int i = 0; i < CPUMemory.agentsCount; ++i) {
        threads.emplace_back([&, i]() {
            MemoryPointers local;
            fillLocalMemory(CPUMemory, i, local);
            processAgentCollisionsCPU(CPUMemory, local, b, i);
            });
    }

    for (auto& t : threads) {
        t.join();
    }
    writeMinimalPath(CPUMemory);
}

Info compute_cpu_primitive(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    parallelAStarPrimitive(m);
    auto end_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
    return result;
}

Info compute_cpu_primitive_one_thread(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    processAgentCollisionsCPUOneThread(m->CPUMemory);
    auto end_time = std::chrono::high_resolution_clock::now();
    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
    return result;
}


Info computeCPU(Map* m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    MemoryPointers& globalMemory = m->CPUMemory;
    int agentsCount = globalMemory.agentsCount;
    for (int agentID = 0; agentID < agentsCount; agentID++) {
        MemoryPointers localMemory;
        fillLocalMemory(globalMemory, agentID, localMemory);
        moveAgentForIndex(agentID, localMemory);
    }
    CTNode root;
    root.paths.resize(m->CPUMemory.agentsCount);
    root.constraints.resize(m->CPUMemory.agentsCount);
    for (int agentID = 0; agentID < m->CPUMemory.agentsCount; agentID++) {
        root.paths[agentID] = ComputeASTAR(m, agentID, {});
    }
    std::vector<std::vector<Position>> solution = resolveConflictsCBS(m, root);
    auto end_time = std::chrono::high_resolution_clock::now();
    pushVector(solution, m);
    auto copy_time = std::chrono::high_resolution_clock::now();
    writeMinimalPath(globalMemory);
    auto minimal_time = std::chrono::high_resolution_clock::now();

    Info result{ 0,0 };
    result.timeRun = std::chrono::duration<double>(end_time - start_time).count() + std::chrono::duration<double>(minimal_time - copy_time).count();
    result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
    return result;
}

namespace internal {
    static int countStepProcessor = 0, countStepGraphicCard = 0;
}

inline Info addInfo(Info toAdd, Info add) {
    Info result{ 0,0 };
    result.timeRun = toAdd.timeRun + add.timeRun;
    result.timeSynchronize = toAdd.timeSynchronize + add.timeSynchronize;
    return result;
}

Info checksynchronizeGPU(Map* m) {
    Info result{ 0,0 };
    if (internal::countStepProcessor > internal::countStepGraphicCard) {
        internal::countStepGraphicCard = internal::countStepProcessor;
        auto start_time = std::chrono::high_resolution_clock::now();
        synchronizeGPUFromCPU(m);
        auto end_time = std::chrono::high_resolution_clock::now();
        result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
    }
    internal::countStepGraphicCard++;
    return result;
}

Info checksynchronizeCPU(Map* m) {
    Info result{ 0,0 };
    if (internal::countStepProcessor < internal::countStepGraphicCard) {
        internal::countStepProcessor = internal::countStepGraphicCard;
        auto start_time = std::chrono::high_resolution_clock::now();
        synchronizeGPUFromCPU(m);
        auto end_time = std::chrono::high_resolution_clock::now();
        result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
    }
    internal::countStepProcessor++;
    return result;
}

Info runOnGraphicCard(Map* m) {
    Info result = checksynchronizeGPU(m);
    Info resultCompute = computeSYCL(m);
    addInfo(result, resultCompute);
    auto start_time = std::chrono::high_resolution_clock::now();
    synchronizeCPUFromGPU(m);
    auto end_time = std::chrono::high_resolution_clock::now();
    result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
    internal::countStepProcessor = internal::countStepGraphicCard;
    return result;
}

Info runHybrid(Map* m) {
    Info result = checksynchronizeGPU(m);
    Info resultCompute = computeHYBRID(m);
    result = addInfo(result, resultCompute);
    return result;
}

Info runPure(Map* m) {
    Info result = checksynchronizeCPU(m);
    Info resultCompute = compute_cpu_primitive(m);
    return addInfo(result, resultCompute);
}

Info runPureOneThread(Map* m) {
    Info result = checksynchronizeCPU(m);
    Info resultCompute = compute_cpu_primitive_one_thread(m);
    return addInfo(result, resultCompute);
}

Info runHigh(Map* m) {
    Info result = checksynchronizeCPU(m);
    Info resultCompute = computeCPU(m);
    return addInfo(result, resultCompute);
}

Info letCompute(ComputeType device, Map* m) {
    switch (device) {
    case ComputeType::pureGraphicCard:
        return runOnGraphicCard(m);
    case ComputeType::hybridGPUCPU:
        return runHybrid(m);
    case ComputeType::pureProcesor:
        return runPure(m);
    case ComputeType::pureProcesorOneThread:
        return runPureOneThread(m);
    case ComputeType::highProcesor:
        return runHigh(m);
    default:
        break;
    }
    return runHigh(m);
}

*/