#include "solve_conflicts.h"
#include "map.h"
#include "heap_primitive.h"
#include <limits>
#include <thread> 
#include "fill_init_convert.h"
#include "a_star_algo.h"
#include "compute.h"

SYCL_EXTERNAL bool shouldYield(int agentId, int conflictAgentId, Agent* agents) {
    Agent& a = agents[agentId], & b = agents[conflictAgentId];
    if (a.direction != b.direction) {
        return (a.direction == AGENT_LOADER);
    }
    if (a.sizePath != b.sizePath) {
        return (a.sizePath > b.sizePath);
    }
    return (agentId > conflictAgentId);
}

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
/*
q w e r g v c x o - agent B
a s d f g t z u i - agent A
=======
kolízia g-g

ale keby
q w e r g v c x o - agent B
s a q w e r t g v - agent C
a s d f g t z u i - agent A
=======
kolízia s-a

tak
a (a) s d f g t z u i - agent A
q   w e r g v c x o - agent B
žiadna kolízia
a keby 
q w e r g v c x o - agent B
q w e r g v c x o - agent D
========
kolízia q-q


a (a) s d f g t z u i - agent B
q (q) w e r g v c x o - agent A
kolízia g-g
*/

SYCL_EXTERNAL bool checkCollision(int agentId, int& conflictAgentId, int t, Constraint& conflictInfo, const MemoryPointers& globalMemory) {
    const int mapSize = globalMemory.width * globalMemory.height;
    Agent* allAgents = globalMemory.agents;
    Position* pathAgentA = &globalMemory.agentPaths[agentId * mapSize];
    int pathSizeA = allAgents[agentId].sizePath;
    if (t >= pathSizeA) return false;
    Position currentPosA = pathAgentA[t];
    if (t > 0) { if (isSamePosition(currentPosA, pathAgentA[t - 1])) { return false; } }
    if (t+1 < pathSizeA) { if (isSamePosition(currentPosA, pathAgentA[t + 1])) { return false; } }
    for (int otherId = 0; otherId < globalMemory.agentsCount; ++otherId) {
        if (otherId == agentId) continue;

        int pathSizeB = allAgents[otherId].sizePath;
        if (pathSizeB <= t) continue;

        Position* pathAgentB = &globalMemory.agentPaths[otherId * mapSize];
        Position currentPosB = pathAgentB[t];
		if (t > 0) { if (isSamePosition(currentPosB, pathAgentB[t - 1])) { continue; } }
		if (t + 1 < pathSizeB) { if (isSamePosition(currentPosB, pathAgentB[t + 1])) { continue; } }
        if (isSamePosition(currentPosA, currentPosB)) {
            conflictAgentId = otherId;
            conflictInfo = {};
            conflictInfo.position = currentPosA;
            conflictInfo.timeStep = t;
            return true;
        }
        if ((t + 1 < pathSizeA) && (t + 1 < pathSizeB)) {
            Position nextPosA = pathAgentA[t + 1];
            Position nextPosB = pathAgentB[t + 1];
            if (isSamePosition(currentPosA, nextPosB) && isSamePosition(currentPosB, nextPosA)) {
                conflictAgentId = otherId;
                conflictInfo = {};
                conflictInfo.position = nextPosA;
                conflictInfo.timeStep = t;
                return true;
            }
        }
    }
    return false;
}


SYCL_EXTERNAL bool applyPathShift(MemoryPointers& localMemory, Constraint& conflictInfo, int agentID) {
	Agent& a = localMemory.agents[agentID];
	int pathSize = a.sizePath;
	if (pathSize + 1 >= localMemory.width*localMemory.height) {	return false; }
	int t = conflictInfo.timeStep;
	if (t >= pathSize || t < 0) { return false;	}
    Position* paths = localMemory.agentPaths;
    for (int moveIdx = pathSize; moveIdx > t; moveIdx--) { 
        paths[moveIdx] = paths[moveIdx-1];
    }
    paths[t] = conflictInfo.position;
	a.sizePath++;
	return true;
}

SYCL_EXTERNAL void processAgentCollisionsGPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, sycl::nd_item<1> item) {
    int agentID = item.get_local_id(0);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        err_flag(globalMemory.minSize_maxtimeStep_error[ERROR]);
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device,
        sycl::access::address_space::global_space>
        maxT(globalMemory.minSize_maxtimeStep_error[MAXTIMESTEP]);
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
        err_flag.store(agentID + 1);
    }
    item.barrier();
    if (err_flag.load() != 0) {
        return;
    }
    Agent& a = localMemory.agents[agentID];
    a.sizePath = reconstructPath(agentID, localMemory);
    if (agentID == 0) {
        maxT.store(max_path(localMemory));
    }
    for (int t = 0; ; t++) {
        item.barrier();
        if (err_flag.load() != 0) {
            return;
        }
        int maxTValue = maxT.load();

        if (t >= maxTValue) {
            return;
        }
        int conflictAgentId = -1, was = 0;
        Constraint conflictFuturePos = { { -1, -1 }, -1 };
        if (a.sizePath > t) {
            if (checkCollision(agentID, conflictAgentId, t, conflictFuturePos, globalMemory)) {
                if (shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                    was = 1;
                }
            }
        }
        item.barrier();
        if (was == 1) {
            bool done = applyPathShift(localMemory, conflictFuturePos, agentID);
            if (done) {
                maxT.fetch_max(a.sizePath);
            }
            else {
                err_flag.store(agentID + 1);
            }
        }
        item.barrier();
    }
}

void processAgentCollisionsCPU(const MemoryPointers& globalMemory, MemoryPointers& localMemory, MyBarrier& item, int agentID) {
    std::atomic<int>& maxT = reinterpret_cast<std::atomic<int>&>(globalMemory.minSize_maxtimeStep_error[MAXTIMESTEP]);
    std::atomic<int>& err_flag = reinterpret_cast<std::atomic<int>&>(globalMemory.minSize_maxtimeStep_error[ERROR]);	
    if (!astar_algorithm_get_sussces(agentID, localMemory)) {
        err_flag.store(agentID + 1);
    }
    item.barrier();
    if (err_flag.load() != 0) {
        return;
    }
    Agent& a = localMemory.agents[agentID];
    a.sizePath = reconstructPath(agentID, localMemory);
	if (agentID == 0) {
		maxT.store(max_path(localMemory));
	}
    for (int t = 0; ; t++) {
        item.barrier();
        if (err_flag.load() != 0) {
            return;
        }
		int maxTValue = maxT.load();

        if (t >= maxTValue) {
            return;
        }
        int conflictAgentId = -1, was = 0;
        Constraint conflictFuturePos = { { -1, -1 }, -1 };
        if (a.sizePath > t) {
            if (checkCollision(agentID, conflictAgentId, t, conflictFuturePos, globalMemory)) {
                if (shouldYield(agentID, conflictAgentId, localMemory.agents)) {
                    was = 1;
                }
            }
        }
        //std::cout << "agentID: " << agentID << " t: " << t << " maxTValue: " <<
        //    maxTValue << std::endl;
        item.barrier();
        if (was == 1) {
            bool done = applyPathShift(localMemory, conflictFuturePos, agentID);
            if (done) {
                int old = maxT.load(), value = a.sizePath;
                std::cout << "old: " << old << " value: " << value << std::endl;
                while (old < value && !maxT.compare_exchange_strong(old, value)) {}
                std::cout << "old: " << old << " value: " << value << std::endl;
			}
			else {
				err_flag.store(agentID + 1);
            }
        }
        item.barrier();
    }
}

void processAgentCollisionsCPUOneThread(const MemoryPointers& globalMemory) {
    int agentCount = globalMemory.agentsCount;
    std::vector<MemoryPointers> localMemorys(agentCount);

    for (int agentID = 0; agentID < agentCount; agentID++) {
        MemoryPointers& localMemory = localMemorys[agentID];
        fillLocalMemory(globalMemory, agentID, localMemory);
        moveAgentForIndex(agentID, localMemory);
    }

    for (int agentID = 0; agentID < agentCount; agentID++) {
        MemoryPointers& localMemory = localMemorys[agentID];
        fillLocalMemory(globalMemory, agentID, localMemory);
        if (!astar_algorithm_get_sussces(agentID, localMemory)) {
            globalMemory.minSize_maxtimeStep_error[ERROR] = agentID + 1;
            return;
        }
        localMemory.agents[agentID].sizePath = reconstructPath(agentID, localMemory);
    }

    int maxT = 0;
    for (int agentID = 0; agentID < agentCount; agentID++) {
        maxT = std::max(maxT, max_path(localMemorys[agentID]));
    }

    for (int t = 0; t < maxT; t++) {
        for (int agentID = 0; agentID < agentCount; agentID++) {
            MemoryPointers& localMemory = localMemorys[agentID];
            Agent& a = localMemory.agents[agentID];
            int& pathSize = a.sizePath;

            if (t >= pathSize) {
                continue; 
            }

            int conflictAgentId = -1;
            Constraint conflictFuturePos = { { -1, -1 }, -1 };

            if (checkCollision(agentID, conflictAgentId, t, conflictFuturePos, globalMemory) &&
                shouldYield(agentID, conflictAgentId, localMemory.agents)) {

                bool done = applyPathShift(localMemory, conflictFuturePos, agentID);
                if (!done) {
                    globalMemory.minSize_maxtimeStep_error[ERROR] = agentID + 1;
                    return;
                }
                pathSize = a.sizePath;
                maxT = std::max(maxT, pathSize);
            }
        }
    }
    writeMinimalPath(globalMemory);
}
