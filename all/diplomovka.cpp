// struct
/*
#define AGENT_UNLOADER 1
#define AGENT_LOADER 2

#define LOADER_SYMBOL 'I'
#define UNLOADER_SYMBOL 'O'
#define OBSTACLE_SYMBOL '#'
#define FREEFIELD_SYMBOL '.'

// nno map symbol
#define AGENT_SYMBOL 'A'

struct Position {
    int x, y;
};

struct Agent {
    int x = 0, y = 0, sizePath = 0;
    int unloaderCurrent;
    int loaderCurrent;
    unsigned char direction;
};

struct Constrait {
    int index, x, y;
};

struct MemoryPointers{
    int width, height, loaderCount, unloaderCount, agentCount;
    char* grid = nullptr;
    Position* loaderPosition = nullptr;
    Position* unloaderPosition = nullptr;
    Agent* agents = nullptr;

    int* minSize = nullptr;
    Position* pathsAgent = nullptr;

    int* gCost = nullptr;
    int* fCost = nullptr;
    Position* cameFrom = nullptr;
    bool* visited = nullptr;

    Position* openList = nullptr;
    Constrait* constrait = nullptr;
    int* numberConstrait = nullptr;
};

struct MyHeap {
    Position* heap;
    int size;
};
// helper function

inline void push(MyHeap& myHeap, Position p, int* gCost, int width) {
    myHeap.heap[myHeap.size] = p;
    myHeap.size++;
    int i = myHeap.size - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (gCost[myHeap.heap[parent].y * width + myHeap.heap[parent].x] <=
            gCost[myHeap.heap[i].y * width + myHeap.heap[i].x]) {
            break;
        }

        Position temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[parent];
        myHeap.heap[parent] = temp;
        i = parent;
    }
}

inline Position pop(MyHeap& myHeap, int* gCost, int width) {
    if (myHeap.size == 0) {
        return { -1, -1 };
    }
    Position root = myHeap.heap[0];
    --(myHeap.size);
    myHeap.heap[0] = myHeap.heap[myHeap.size];

    int i = 0;
    while (2 * i + 1 < myHeap.size) {
        int left = 2 * i + 1, right = 2 * i + 2;
        int smallest = left;

        if (right < myHeap.size &&
            gCost[myHeap.heap[right].y * width + myHeap.heap[right].x] <
            gCost[myHeap.heap[left].y * width + myHeap.heap[left].x]) {
            smallest = right;
        }

        if (gCost[myHeap.heap[i].y * width + myHeap.heap[i].x] <=
            gCost[myHeap.heap[smallest].y * width + myHeap.heap[smallest].x]) {
            break;
        }
        Position temp = myHeap.heap[i];
        myHeap.heap[i] = myHeap.heap[smallest];
        myHeap.heap[smallest] = temp;
        i = smallest;
    }

    return root;
}

inline bool empty(MyHeap& myHeap) {
    return myHeap.size == 0;
}


inline int getTrueIndexGrid(int width, int x, int y) {
    return y * width + x;
}

namespace internal {

    inline void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory) {
        localMemory.width = globalMemory.width;
        localMemory.height = globalMemory.height;
        localMemory.agentCount = globalMemory.agentCount;
        localMemory.loaderCount = globalMemory.loaderCount;
        localMemory.unloaderCount = globalMemory.unloaderCount;

        const int mapSize = localMemory.width * localMemory.height;
        int offset = mapSize * agentId;
        localMemory.grid = globalMemory.grid;
        localMemory.loaderPosition = globalMemory.loaderPosition;
        localMemory.unloaderPosition = globalMemory.unloaderPosition;
        localMemory.agents = globalMemory.agents;
        localMemory.minSize = globalMemory.minSize;
        localMemory.pathsAgent = &(globalMemory.pathsAgent[offset]);
        localMemory.gCost = &(globalMemory.gCost[offset]);
        localMemory.fCost = &(globalMemory.fCost[offset]);
        localMemory.cameFrom = &(globalMemory.cameFrom[offset]);
        localMemory.visited = &(globalMemory.visited[offset]);
        localMemory.openList = &(globalMemory.openList[offset]);
        localMemory.constrait = &(globalMemory.constrait[offset / 4]);
        localMemory.numberConstrait = globalMemory.numberConstrait;
    }

    inline void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start) {
        const int width = localMemory.width;
        const int height = localMemory.height;
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
}

inline int myAbs(int input) {
    if (input < 0) {
        return -input;
    }
    return input;
}
// a* algoritmus

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
        Position start = Position{ a.x, a.y };
        Position goal;
        if (a.direction == AGENT_LOADER) {
            goal = localMemory.loaderPosition[a.loaderCurrent];
        }
        else {
            goal = localMemory.unloaderPosition[a.unloaderCurrent];
        }
        MyHeap myHeap{ localMemory.openList, 0 };
        initializeCostsAndHeap(localMemory, myHeap, start);
        runAStar(localMemory, myHeap, start, goal);
        localMemory.agents[agentId].sizePath = reconstructPath(localMemory, start, goal);
    }
}
// solve conflicts

namespace internal {

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
                conflictFuturePos.index = t;
                conflictFuturePos.x = otherNextPos.x;
                conflictFuturePos.y = otherNextPos.y;
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

    inline int runAStarContrait(Position start, Position goal, MemoryPointers& localMemory) {
        MyHeap myHeap{ localMemory.openList, 0 };
        initializeCostsAndHeap(localMemory, myHeap, start);

        while (!empty(myHeap)) {
            Position current = pop(myHeap, localMemory.fCost, localMemory.width);
            int trueIndex = getTrueIndexGrid(localMemory.width, current.x, current.y);

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

                int newG = localMemory.gCost[trueIndex] + 1;
                if (newG < localMemory.gCost[nextTrueIndex]) {
                    localMemory.gCost[nextTrueIndex] = newG;
                    localMemory.fCost[nextTrueIndex] = newG + abs(goal.x - next.x) + abs(goal.y - next.y);
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



    inline int findAlternativePath(Position currentPos, Position targetPos, Position* foundPath, MemoryPointers& localMemory) {
        const int MAX_PATH = 25;
        const int SEARCH_SIZE = 5;

        Position queue[MAX_PATH];
        int queueIndex[MAX_PATH];
        int queueStart = 0, queueEnd = 0;
        Position cameFrom[MAX_PATH];
        bool visited[SEARCH_SIZE][SEARCH_SIZE];
        for (int i = 0; i < SEARCH_SIZE; i++) {
            for (int j = 0; j < SEARCH_SIZE; j++) {
                visited[i][j] = false;
            }
        }

        queue[queueEnd] = currentPos;
        queueIndex[queueEnd] = currentIndex;
        visited[SEARCH_SIZE / 2][SEARCH_SIZE / 2] = true;
        queueEnd++;

        Position directions[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
        int foundPathSize = 0;

        while (queueStart < queueEnd && queueEnd < 25) {
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

                if (next.x < 0 || next.x >= localMemory.width || next.y < 0 || next.y >= localMemory.height) continue;

                int localX = next.x - (currentPos.x - SEARCH_SIZE / 2);
                int localY = next.y - (currentPos.y - SEARCH_SIZE / 2);

                if (localX < 0 || localX >= SEARCH_SIZE || localY < 0 || localY >= SEARCH_SIZE) continue;
                if (visited[localY][localX]) continue;

                bool isBlocked = false;
                for (int c = 0; c < numConstraints; c++) {
                    if (localMemory.constrait[c].x == next.x && localMemory.constrait[c].y == next.y && localMemory.constrait[c].index == nextIndex) {
                        isBlocked = true;
                        break;
                    }
                }
                if (isBlocked) continue;

                if (localMemory.grid[getTrueIndexGrid(localMemory.width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;

                queue[queueEnd] = next;
                queueIndex[queueEnd] = nextIndex;
                cameFrom[queueEnd] = current;
                visited[localY][localX] = true;
                queueEnd++;
            }
        }
        return -1;
    }

    inline void applyPathShift(Position* paths, int agentSizePath, int t, Position* foundPath, int foundPathSize) {
        for (int moveIdx = agentSizePath - 1; moveIdx > t; moveIdx--) {
            paths[moveIdx + foundPathSize] = paths[moveIdx];
        }
        for (int i = 0; i < foundPathSize; i++) {
            paths[t + 1 + i] = foundPath[foundPathSize - 1 - i];
        }
    }

    bool handleCollision(int agentId, int conflictAgentId, int timeStep, Position& currentPos,
        Position& nextGoal, MemoryPointers& localMemory) {

        if (!shouldYield(agentId, conflictAgentId, localMemory.agents)) {
            return false;
        }
        int indexTopConstrait = localMemory.numberConstrait[agentId];
        localMemory.constrait[indexTopConstrait] = { nextGoal.x, nextGoal.y, timeStep };
        localMemory.numberConstrait[agentId] += 1;

        Position foundPath[25];
        int foundPathSize = findAlternativePath(currentPos, nextGoal, foundPath, timeStep, localMemory);

        if (foundPathSize > 0) {
            applyPathShift(localMemory.pathsAgent, localMemory.agents[agentId].sizePath, timeStep, foundPath, foundPathSize);
            localMemory.agents[agentId].sizePath += foundPathSize;
            return true;
        }
        else {
            Agent& a = localMemory.agents[agentId];
            int pathSize =
                foundPath = runAStarContrait({ a.x, a.y }, );
        }

        return false;
    }

    void processAgentCollisionsGPU(sycl::nd_item<1> item, MemoryPointers& localMemory, const MemoryPointers& globalMemory) {
        int agentId = item.get_local_id(0);  // Každý agent dostane svoje ID v rámci work-groupy
        const int agentCount = localMemory.agentCount;

        MyHeap myHeap{ localMemory.openList, 0 };

        for (int t = 0; t < localMemory.agents[agentId].sizePath - 1; t++) {
            item.barrier();
            int conflictAgentId = -1;
            Constrait conflictFuturePos = { -1, -1, -1 };

            if (!checkCollision(agentId, conflictAgentId, t, conflictFuturePos, globalMemory)) {
                continue;
            }

            handleCollision(agentId, conflictAgentId,
                currentPos, nextPos,
                t, myHeap, localMemory);
        }
        sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
            sycl::access::address_space::global_space>  atomicMin(*localMemory.minSize);
        atomicMin.fetch_min(localMemory.agents[agentId].sizePath);
    }
}*/