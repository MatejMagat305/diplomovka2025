#include "compute.h"
#include "map.h" 

SYCL_EXTERNAL void lookRemoveOldColisions(int agentID, MemoryPointers& localMemory, int moveSize) {
	int count = localMemory.numberConstraints[agentID];
	if (count == 0) return; 
	Constraint* constraints = localMemory.constraints;
	int newCount = 0;
	for (int i = 0; i < count; i++) {
		Constraint c = constraints[i];
		c.timeStep = c.timeStep - moveSize;
		if (c.timeStep >= 0){
			constraints[newCount] = c;
			newCount++;
		}
	}
	localMemory.numberConstraints[agentID] = newCount;
}

SYCL_EXTERNAL void moveAgentForIndex(int id, MemoryPointers& localMemory) {
	Agent& a = localMemory.agents[id];
	a.mustWait = 0;
	const int moveSize = localMemory.minSize_numberColision_error[MINSIZE];
	int agentSizePath = localMemory.agents[id].sizePath;
	if (moveSize <= 0 || moveSize > agentSizePath){
		return;
	}
	if (moveSize  == agentSizePath){
		if (a.direction == AGENT_LOADER) {
			Position& loader = localMemory.loaderPositions[a.loaderCurrent];
			a.direction = AGENT_UNLOADER;
			a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % localMemory.loadersCount;
			// TODO random
			a.sizePath = 0;
			a.position = loader;
			a.goal = localMemory.unloaderPositions[a.unloaderCurrent];
			localMemory.numberConstraints[id] = 0;
			return;
		}
		else if (a.direction == AGENT_UNLOADER){
			Position& unloader = localMemory.unloaderPositions[a.unloaderCurrent];
			a.direction = AGENT_LOADER;
			a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % localMemory.unloadersCount;
			a.sizePath = 0;
			a.position = unloader;
			a.goal = localMemory.loaderPositions[a.loaderCurrent];
			localMemory.numberConstraints[id] = 0;
			return;
		}
		else if (a.direction == AGENT_PARK_UNLOADER) {
			a.direction = AGENT_UNLOADER;
			a.sizePath = 0;
			a.position = localMemory.agentPaths[moveSize - 1];
			a.goal = localMemory.unloaderPositions[a.unloaderCurrent];
			localMemory.numberConstraints[id] = 0;
			return;
		}
		else {
			a.direction = AGENT_LOADER;
			a.sizePath = 0;
			a.position = localMemory.agentPaths[moveSize - 1];
			a.goal = localMemory.loaderPositions[a.loaderCurrent];
			localMemory.numberConstraints[id] = 0;
			return;
		}
	}
	a.position = localMemory.agentPaths[moveSize - 1];

	for (int i = moveSize; i < agentSizePath; i++) {
		localMemory.agentPaths[i - moveSize] = localMemory.agentPaths[i];
	}
	localMemory.agents[id].sizePath = localMemory.agents[id].sizePath - moveSize;
	lookRemoveOldColisions(id, localMemory, moveSize);
}