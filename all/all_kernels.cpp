
// uutils.cpp

#include "sycl/sycl.hpp"


#define AGENT_UNLOADER 1
#define INF 1000000
#define AGENT_LOADER 2

#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

void lookRemoveOldColisions(int agentID, MemoryPointers& localMemory, int moveSize) {
    int count = localMemory.numberConstraits[agentID];
    if (count == 0) return;
    Constrait* constraints = localMemory.constraits;
    int newCount = 0;
    for (int i = 0; i < count; i++) {
        Constrait c = constraints[i];
        c.timeStep = c.timeStep - moveSize;
        if (c.timeStep >= 0) {
            constraints[newCount] = c;
            newCount++;
        }
    }
    localMemory.numberConstraits[agentID] = newCount;
}

void moveAgentForIndex(int id, MemoryPointers& localMemory) {
    Agent& a = localMemory.agents[id];
    const int moveSize = *localMemory.minSize_numberColision;
    if (moveSize <= 0) {
        return;
    }
    int agentSizePath = localMemory.agents[id].sizePath;
    if (moveSize == agentSizePath) {
        if (a.direction == AGENT_LOADER) {
            Position& loader = localMemory.loaderPosition[a.loaderCurrent];
            a.direction = AGENT_UNLOADER;
            a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % localMemory.loaderCount;
            // TODO random
            a.sizePath = 0;
            a.x = loader.x;
            a.y = loader.y;
            localMemory.numberConstraits[id] = 0;
            return;
        }
        else {
            Position& unloader = localMemory.unloaderPosition[a.unloaderCurrent];
            a.direction = AGENT_LOADER;
            a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % localMemory.unloaderCount;
            a.sizePath = 0;
            a.x = unloader.x;
            a.y = unloader.y;
            localMemory.numberConstraits[id] = 0;
            return;
        }
    }
    Position& p = localMemory.agentPaths[moveSize - 1];
    a.x = p.x;
    a.y = p.y;

    for (int i = moveSize; i < agentSizePath; i++) {
        localMemory.agentPaths[i - moveSize] = localMemory.agentPaths[i];
    }
    localMemory.agents[id].sizePath = localMemory.agents[id].sizePath - moveSize;
    lookRemoveOldColisions(id, localMemory, moveSize);
}
struct MemoryPointers {
    int width, height, loadersCount, unloadersCount, agentsCount;
    char* grid = nullptr;
    Position* loaderPositions = nullptr;
    Position* unloaderPositions = nullptr;
    Agent* agents = nullptr;

    int* minSize_numberColision = nullptr;
    Position* agentPaths = nullptr;
    Position* pathsAgent2 = nullptr;

    int* gCost = nullptr;
    int* fCost_rhs = nullptr;
    Position* cameFrom = nullptr;
    bool* visited = nullptr;

    HeapPositionNode* openList = nullptr;
    Constrait* constraits = nullptr;
    int* numberConstraits = nullptr;
};

class Map {
public:
    MemoryPointers CPUMemory;
};

struct HeapPositionNode {
    Position pos;
    int priority;
};

struct positionHeap {
    HeapPositionNode* heap;
    int size;
};


struct Position {
    int x, y;
};

struct Agent {
    int x = 0, y = 0, sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};

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

#define MOVE_CONSTRATINT 1
struct Constrait {
    Position to;
    int type, timeStep;
};

int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}

int myAbs(int input) {
    int mask = input >> 31;  // Vytvorí -1 pre záporné èísla, 0 pre kladné
    return (input + mask) ^ mask;
}

int reconstructPath(int agentID, MemoryPointers& localMemory, Position start, Position goal) {
    const int width = localMemory.width;
    const int height = localMemory.height;
    const int mapSize = width * height;
    int pathLength = 0;
    Position current = goal;
    localMemory.agentPaths[pathLength++] = current;
    for (int i = 0; i < mapSize; i++) {
        int currentIndex = getTrueIndexGrid(width, current.x, current.y);
        if (currentIndex < 0 || currentIndex >= mapSize) {
            break;
        }
        Position next = localMemory.cameFrom[currentIndex];
        if (next.x == -1 || next.y == -1) {
            break;
        }
        current = next;
        localMemory.agentPaths[pathLength++] = current;
        if (current.x == start.x && current.y == start.y) {
            break;
        }
    }
    if (getTrueIndexGrid(width, current.x, current.y) < 0) {
        return 0;
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
    localMemory.minSize_numberColision = globalMemory.minSize_numberColision;
    localMemory.agentPaths = &(globalMemory.agentPaths[offset]);
    localMemory.gCost = &(globalMemory.gCost[offset]);
    localMemory.fCost_rhs = &(globalMemory.fCost_rhs[offset]);
    localMemory.cameFrom = &(globalMemory.cameFrom[offset]);
    localMemory.visited = &(globalMemory.visited[offset]);
    localMemory.openList = &(globalMemory.openList[offset]);
    localMemory.constraits = &(globalMemory.constraits[offset / 4]);
    localMemory.numberConstraits = globalMemory.numberConstraits;
}

void setGCostFCostToINF(int mapSize, MemoryPointers& localMemory) {
    for (int i = 0; i < mapSize; i++) {
        localMemory.gCost[i] = INF;
        localMemory.fCost_rhs[i] = INF;
    }
}

void initializeCostsAndHeap(MemoryPointers& localMemory, positionHeap& myHeap, Position start) {
    const int width = localMemory.width;
    const int height = localMemory.height;
    const int mapSize = width * height;
    for (int i = 0; i < mapSize; i++) {
        localMemory.visited[i] = false;
    }
    setGCostFCostToINF(mapSize, localMemory);
    int index = getTrueIndexGrid(width, start.x, start.y);
    localMemory.gCost[index] = 0;
    localMemory.fCost_rhs[index] = 0;
    push(myHeap, start, localMemory.fCost_rhs[index]);
}


std::vector<std::vector<Position>> getVector(Map& m) {
    size_t offset = m.CPUMemory.width * m.CPUMemory.height;
    std::vector<std::vector<Position>> result(m.CPUMemory.agentsCount);

    Position* ptr = m.CPUMemory.agentPaths;
    for (int i = 0; i < m.CPUMemory.agentsCount; ++i) {
        size_t pathSize = m.CPUMemory.agents[i].sizePath;
        result[i].assign(ptr, ptr + pathSize);  // Skopírujeme dáta do vektora
        ptr += offset;  // Posun na ïalšieho agenta
    }
    return result;
}


void pushVector(const std::vector<std::vector<Position>>& vec2D, Map& m) {
    size_t offset = m.CPUMemory.width * m.CPUMemory.height;
    Position* ptr = m.CPUMemory.agentPaths;
    int minSize = vec2D[0].size();
    for (int i = 0; i < m.CPUMemory.agentsCount; ++i) {
        const std::vector<Position>& row = vec2D[i];
        std::copy(row.begin(), row.end(), ptr);
        ptr += offset;
        int agentSizePath = row.size();
        m.CPUMemory.agents[i].sizePath = agentSizePath;
        if (agentSizePath < minSize) {
            minSize = agentSizePath;
        }
    }
    *m.CPUMemory.minSize_numberColision = minSize;
}

bool isSamePosition(const Position& a, const Position& b) {
    return a.x == b.x && a.y == b.y;
}

int ManhattanHeuristic(const Position& a, const Position& b) {
    return myAbs(a.x - b.x) + myAbs(a.y - b.y);
}


struct Key {
    int first;
    int second;
};

// d_star_algo.cpp

Key computeKey(MemoryPointers& localMemory, Position s, Position start) {
    int index = getTrueIndexGrid(localMemory.width, s.x, s.y);
    int minVal = std::min(localMemory.gCost[index], localMemory.fCost_rhs[index]);
    Key key;
    key.first = minVal + ManhattanHeuristic(s, start);
    key.second = minVal;
    return key;
}

bool keyLess(const Key& a, const Key& b) {
    return (a.first < b.first) || ((a.first == b.first) && (a.second < b.second));
}

void updateVertex_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position s, Position start) {
    int index = getTrueIndexGrid(localMemory.width, s.x, s.y);

    bool blocked = (localMemory.grid[index] == OBSTACLE_SYMBOL);
    for (int i = 0; i < localMemory.numberConstraits[agentID] && !blocked; i++) {
        Constrait c = localMemory.constraits[i];
        if (c.to.x == s.x && c.to.y == s.y && (c.type & 0b11) == MOVE_CONSTRATINT) {
            blocked = true;
        }
    }
    if (blocked) {
        localMemory.fCost_rhs[index] = INF;
    }
    else {
        int minVal = INF;
        Position neighbors[4] = {
            {s.x + 1, s.y}, {s.x - 1, s.y},
            {s.x, s.y + 1}, {s.x, s.y - 1}
        };
        Position bestPred = { -1, -1 };
        for (int i = 0; i < 4; i++) {
            Position s2 = neighbors[i];
            if (s2.x < 0 || s2.x >= localMemory.width || s2.y < 0 || s2.y >= localMemory.height) {
                continue;
            }
            int s2Index = getTrueIndexGrid(localMemory.width, s2.x, s2.y);
            if (localMemory.grid[s2Index] == OBSTACLE_SYMBOL) {
                continue;
            }
            int cost = 1;
            int candidate = localMemory.gCost[s2Index] + cost;
            if (candidate < minVal) {
                minVal = candidate;
                bestPred = s2;
            }
        }
        localMemory.fCost_rhs[index] = minVal;
        localMemory.cameFrom[index] = bestPred;
    }
    Key key = computeKey(localMemory, s, start);
    push(heap, s, key.first);
}

void computeShortestPath_DStar(int agentID, MemoryPointers& localMemory, positionHeap& heap, Position start, Position goal) {
    int startIndex = getTrueIndexGrid(localMemory.width, start.x, start.y);
    Key startKey = computeKey(localMemory, start, start);

    while (!empty(heap)) {
        Position u = pop(heap);
        int uIndex = getTrueIndexGrid(localMemory.width, u.x, u.y);
        Key uKey = computeKey(localMemory, u, start);
        Key startCurrentKey = computeKey(localMemory, start, start);
        if (!keyLess(startCurrentKey, uKey))
            break;

        if (localMemory.gCost[uIndex] > localMemory.fCost_rhs[uIndex]) {
            localMemory.gCost[uIndex] = localMemory.fCost_rhs[uIndex];
            Position neighbors[4] = {
                {u.x + 1, u.y}, {u.x - 1, u.y},
                {u.x, u.y + 1}, {u.x, u.y - 1}
            };
            for (int i = 0; i < 4; i++) {
                Position s = neighbors[i];
                if (s.x < 0 || s.x >= localMemory.width || s.y < 0 || s.y >= localMemory.height) {
                    continue;
                }
                updateVertex_DStar(agentID, localMemory, heap, s, start);
            }
        }
        else {
            localMemory.gCost[uIndex] = INF;
            updateVertex_DStar(agentID, localMemory, heap, u, start);
            Position neighbors[4] = {
                {u.x + 1, u.y}, {u.x - 1, u.y},
                {u.x, u.y + 1}, {u.x, u.y - 1}
            };
            for (int i = 0; i < 4; i++) {
                Position s = neighbors[i];
                if (s.x < 0 || s.x >= localMemory.width || s.y < 0 || s.y >= localMemory.height) {
                    continue;
                }
                updateVertex_DStar(agentID, localMemory, heap, s, start);
            }
        }
    }
}


void computePathForAgent_DStar(int agentId, MemoryPointers& localMemory) {
    Agent a = localMemory.agents[agentId];
    Position start = { a.x, a.y };
    Position goal;
    if (a.direction == AGENT_LOADER) {
        goal = localMemory.loaderPositions[a.loaderCurrent];
    }
    else {
        goal = localMemory.unloaderPositions[a.unloaderCurrent];
    }

    int mapSize = localMemory.width * localMemory.height;
    int goalIndex = getTrueIndexGrid(localMemory.width, goal.x, goal.y);
    localMemory.fCost_rhs[goalIndex] = 0;
    positionHeap heap = { localMemory.openList, 0 };
    Key goalKey = computeKey(localMemory, goal, start);
    push(heap, start, goalKey.first);
    computeShortestPath_DStar(agentId, localMemory, heap, start, goal);
    localMemory.agents[agentId].sizePath = reconstructPath(agentId, localMemory, start, goal);
}

// solve_conflicts.cpp

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
    int count = localMemory.numberConstraits[agentID], newCount = 0, mapSize = localMemory.height * localMemory.width;
    if (count == 0) return;
    Constrait* constraints = localMemory.constraits;
    Agent* agents = localMemory.agents;
    for (int i = 0; i < count; i++) {
        Constrait c = constraints[i];
        Position p = c.to;
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
        atomicNumber(*globalMemory.minSize_numberColision);
    atomicNumber.fetch_or(1);
    while (true) {
        item.barrier();
        if (atomicNumber.load() == 0) { break; }
        computePathForAgent_DStar(agentID, localMemory);
        item.barrier();
        if (agentID == 0) { atomicNumber.store(0); }
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
                atomicNumber.fetch_or(1);
                break;
            }
        }
    }
}
// run

MemoryPointers GPUMemory{};
sycl::queue& getQueue() {
    static sycl::queue q{ sycl::default_selector_v };
    return q;
}

void SYCL_DStar(Map& m) {
    sycl::queue& q = getQueue();
    int workSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
    // pohyb o minimálnu cestu 
    auto moveTO = q.submit([&](sycl::handler& h) {
        MemoryPointers globalMemory = GPUMemory;
        h.parallel_for(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), [=](sycl::id<1> idx) {
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
        sycl::nd_range<1> ndRange(sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)), sycl::range<1>(std::min(workSize, m.CPUMemory.agentsCount)));
        h.parallel_for(ndRange, [=](sycl::nd_item<1> item) {
            const int id = item.get_global_id(0);
            MemoryPointers localMemory;
            fillLocalMemory(globalMemory, id, localMemory);
            setGCostFCostToINF(localMemory.width * localMemory.height, localMemory);
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
