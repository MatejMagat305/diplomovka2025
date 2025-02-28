#include "compute.h"
#include "a_star_algo.h"
#include "map.h" 

void moveAgentForIndex(int id, MemoryPointers& localMemory) {
	Agent& a = localMemory.agents[id];
	const int moveSize = *localMemory.minSize;
	if (moveSize <= 0){
		return;
	}
	int agentSizePath = localMemory.agents[id].sizePath;
	if (moveSize  == agentSizePath){
		if (a.direction == AGENT_LOADER) {
			Position& loader = localMemory.loaderPosition[a.loaderCurrent];
			a.direction = AGENT_UNLOADER;
			a.loaderCurrent = (a.loaderCurrent + 1 + moveSize) % localMemory.loaderCount;
			// TODO random
			a.sizePath = 0;
			a.x = loader.x;
			a.y = loader.y;
			return;
		}
		else {
			Position& unloader = localMemory.unloaderPosition[a.unloaderCurrent];
			a.direction = AGENT_LOADER;
			a.unloaderCurrent = (a.unloaderCurrent + 1 + moveSize) % localMemory.unloaderCount;
			a.sizePath = 0;
			a.x = unloader.x;
			a.y = unloader.y;
			return;
		}
	}
	Position& p = localMemory.pathsAgent[moveSize - 1];
	a.x = p.x;
	a.y = p.y;

	for (int i = moveSize; i < agentSizePath; i++) {
		localMemory.pathsAgent[i - moveSize] = localMemory.pathsAgent[i];
	}
	localMemory.agents[id].sizePath = localMemory.agents[id].sizePath - moveSize;
}