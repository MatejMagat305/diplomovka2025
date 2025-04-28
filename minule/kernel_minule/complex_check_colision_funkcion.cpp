
SYCL_EXTERNAL bool checkCollision(int agentId, int& conflictAgentId, int t, Constraint& conflictInfo, const MemoryPointers& globalMemory) {
    const int mapSize = globalMemory.width * globalMemory.height;
    Position* allPaths = globalMemory.agentPaths;
    int* allDelays = globalMemory.numberConstraints;
    Agent* allAgents = globalMemory.agents;

    Position* pathAgentA = &allPaths[agentId * mapSize];
    int delayA = allDelays[agentId];        
    int pathSizeA = allAgents[agentId].sizePath;
    Position currentPosA = pathAgentA[t];
    Position previousPosA = {}; 
    if (t > 0) {
        previousPosA = pathAgentA[t - 1];
    }
    for (int otherId = 0; otherId < globalMemory.agentsCount; ++otherId) {
        if (otherId == agentId) continue;
        int delayB = allDelays[otherId];
        int pathSizeB = allAgents[otherId].sizePath;
		if (pathSizeB + delayB <= t){ continue; }

        Position* pathAgentB = &allPaths[otherId * mapSize];
        int indexCurrentB = t + delayA - delayB;

        Position currentPosB = pathAgentB[indexCurrentB];
		Position previousPosB = {};
		if (indexCurrentB > 0) {
			previousPosB = pathAgentB[indexCurrentB - 1];
		}
		if (t > 0 && indexCurrentB > 0) {
            if (isSamePosition(currentPosA, previousPosB) && isSamePosition(currentPosB, previousPosA)) {
				conflictAgentId = otherId;
				conflictInfo = {};
				conflictInfo.position = currentPosA;
				conflictInfo.timeStep = t - 1;
				return true;
			}
		}
		if (isSamePosition(currentPosA, currentPosB)) {
			conflictAgentId = otherId;
			conflictInfo = {};
			conflictInfo.position = currentPosA;
			conflictInfo.timeStep = t;
			return true;
		}
    } 
    return false;
}
