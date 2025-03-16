#include "compute.h"
#include "map.h" 

void lookRemoveOldColisions(int agentID, MemoryPointers& localMemory, int moveSize) {
	int count = localMemory.numberConstraits[agentID];
	if (count == 0) return; 
	Constraint* constraints = localMemory.constraits;
	int newCount = 0;
	for (int i = 0; i < count; i++) {
		Constraint c = constraints[i];
		c.timeStep = c.timeStep - moveSize;
		if (c.timeStep >= 0){
			constraints[newCount] = c;
			newCount++;
		}
	}
	localMemory.numberConstraits[agentID] = newCount;
}

void moveAgentForIndex(int id, MemoryPointers& localMemory) {
	Agent& a = localMemory.agents[id];
	const int moveSize = *localMemory.minSize_numberColision;
	if (moveSize <= 0){
		return;
	}
	int agentSizePath = localMemory.agents[id].sizePath;
	if (moveSize  == agentSizePath){
		if (a.direction == AGENT_LOADER) {
			Position& loader = localMemory.loaderPositions[a.loaderCurrent];
			a.direction = AGENT_UNLOADER;
			a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % localMemory.loadersCount;
			// TODO random
			a.sizePath = 0;
			a.x = loader.x;
			a.y = loader.y;
			localMemory.numberConstraits[id] = 0;
			return;
		}
		else {
			Position& unloader = localMemory.unloaderPositions[a.unloaderCurrent];
			a.direction = AGENT_LOADER;
			a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % localMemory.unloadersCount;
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