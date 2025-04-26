#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <thread> 
#include "fill_init_convert.h"
#include "a_star_algo.h"
#include "compute.h"

SYCL_EXTERNAL int max_path(MemoryPointers& localMemory) {
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

SYCL_EXTERNAL bool checkCollision(int agentId, int& conflictAgentId, int t, Constraint& conflictFuturePos, const MemoryPointers& globalMemory) {
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

SYCL_EXTERNAL bool findFreeBlockPark(int agentId, const MemoryPointers& globalMemory, MemoryPointers& localMemory){
	return false;
    Agent& a = localMemory.agents[agentId];
    const int width = localMemory.width;
    const int height = localMemory.height;
	
    int numberConstraints = localMemory.numberConstraints[agentId];
    Constraint& c = localMemory.constraints[numberConstraints-1];
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
    for (int i = constraint.timeStep; i < agentOtherSizePath; i++){
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

SYCL_EXTERNAL void lookRemoveOldConstrait(int agentID, const MemoryPointers& globalMemory, MemoryPointers& localMemory) {
    int count = localMemory.numberConstraints[agentID], newCount = 0, mapSize = localMemory.height*localMemory.width;
    if (count == 0) return;
    Constraint* constraints = localMemory.constraints;
    for (int i = 0; i < count; i++){
        Constraint c = constraints[i];
        Position p = c.position;
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
    localMemory.numberConstraints[agentID] = newCount;
}

SYCL_EXTERNAL bool shouldYield(int agentId, int conflictAgentId, Agent* agents) {
    Agent& a = agents[agentId], &b = agents[conflictAgentId];
    if (a.direction == AGENT_PARK_LOADER || a.direction == AGENT_PARK_UNLOADER){
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

SYCL_EXTERNAL void processAgentCollisionsGPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, sycl::nd_item<1> item) {
    int agentID = item.get_local_id(0);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        collision_flag(globalMemory.minSize_numberColision_error[NUMBERCOLISION]);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        err_flag(globalMemory.minSize_numberColision_error[ERROR]);
    localMemory.numberConstraints[agentID] = 0;
    collision_flag.store(1);
    err_flag.store(0);
    bool private_reconstruct_flag = true;
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
		err_flag.store(agentID + 1);
    }
    while (true) {
        item.barrier();
        int collision = collision_flag.load(), error = err_flag.load();
        if (collision == 0 || error != 0) {
            break;
        }
        if (private_reconstruct_flag){
            localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory);
            private_reconstruct_flag = false;
        }
        item.barrier();
        collision_flag.store(0);
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
						err_flag.store(agentID+1);
                        globalMemory.agents[agentID].sizePath = 0;
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
    std::atomic<int>& collision_flag = reinterpret_cast<std::atomic<int>&>(globalMemory.minSize_numberColision_error[NUMBERCOLISION]);
    std::atomic<int>& err_flag = reinterpret_cast<std::atomic<int>&>(globalMemory.minSize_numberColision_error[ERROR]);
	// std::cout << "break point " << agentID << std::endl;
    localMemory.numberConstraints[agentID] = 0;
    collision_flag.store(1);
    bool private_reconstruct_flag = true;
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
		err_flag.store(agentID + 1);
    }
    while (true) {
		// std::cout << "break point 1 - call barrier, start reconstruct, id: " << agentID << std::endl;
        item.barrier();
        int collision = collision_flag.load(), error = err_flag.load();
        if (collision == 0 || error != 0) { 
            break;
        }
        if (private_reconstruct_flag) {
            localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory);
            private_reconstruct_flag = false;
        }
        item.barrier();
		// std::cout << "break point 2 - end reconstruct, id: " << agentID << std::endl;
        collision_flag.store(0);
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
                        globalMemory.agents[agentID].sizePath = 0;
                        break;
                    }
                }
                private_reconstruct_flag = true;
                break;
            }
        }
		// std::cout << "break point 3 - end check collision, id: " << agentID << std::endl;
    }
}

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory) {
    int collision_flag = 1;
    int agentCount = globalMemory.agentsCount;
    std::vector<MemoryPointers> localMemorys(agentCount);
    std::vector<bool> private_reconstruct_flags(agentCount);
    for (int agentID = 0; agentID < agentCount; agentID++) {
        // std::cout << "break point 1 - fill " << agentID << std::endl;
        MemoryPointers& localMemory = localMemorys[agentID];
        fillLocalMemory(globalMemory, agentID, localMemory);
		moveAgentForIndex(agentID, localMemory);
    }

    for (int agentID = 0; agentID < agentCount; agentID++){
        MemoryPointers& localMemory = localMemorys[agentID];
        fillLocalMemory(globalMemory, agentID, localMemory);
        localMemory.numberConstraints[agentID] = 0;
        if (!astar_algorithm_get_sussces(agentID, localMemory)) {
			globalMemory.minSize_numberColision_error[ERROR] = agentID + 1;
        }
        private_reconstruct_flags[agentID] = true;        
    }
    while (collision_flag != 0 && globalMemory.minSize_numberColision_error[ERROR] == 0) {
        collision_flag = 0;
        for (int agentID = 0; agentID < agentCount; agentID++) {
            // std::cout << "break point 2 - reconstruct " << agentID << std::endl;
            if (private_reconstruct_flags[agentID]) {
                localMemorys[agentID].agents[agentID].sizePath = reconstructPath(agentID, localMemorys[agentID]);
                private_reconstruct_flags[agentID] = false;
            }
        }
        for (int agentID = 0; agentID < agentCount; agentID++) {
            // std::cout << "break point 3 - check Colision" << agentID << std::endl;
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
							globalMemory.agents[agentID].sizePath = 0;
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