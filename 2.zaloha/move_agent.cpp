#include "compute.h"
#include "map.h" 

SYCL_EXTERNAL void moveAgentForIndex(int id, MemoryPointers& localMemory) {
	Agent& a = localMemory.agents[id];
	const int moveSize = localMemory.minSize_maxtimeStep_error[MINSIZE];
	int agentSizePath = localMemory.agents[id].sizePath;
	if (moveSize <= 0 || moveSize > agentSizePath){
		return;
	}
	if (moveSize == agentSizePath) {
		if (a.direction == AGENT_LOADER) {
			Position& loader = localMemory.loaderPositions[a.loaderCurrent];
			a.direction = AGENT_UNLOADER;
			a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % localMemory.loadersCount;
			a.position = loader;
			a.goal = localMemory.unloaderPositions[a.unloaderCurrent];
			return;
		}
		else {
			Position& unloader = localMemory.unloaderPositions[a.unloaderCurrent];
			a.direction = AGENT_LOADER;
			a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % localMemory.unloadersCount;
			a.sizePath = 0;
			a.position = unloader;
			a.goal = localMemory.loaderPositions[a.loaderCurrent];
			return;
		}
	}
	a.position = localMemory.agentPaths[moveSize - 1];

	for (int i = moveSize; i < agentSizePath; i++) {
		localMemory.agentPaths[i - moveSize] = localMemory.agentPaths[i];
	}
	localMemory.agents[id].sizePath = localMemory.agents[id].sizePath - moveSize;
}