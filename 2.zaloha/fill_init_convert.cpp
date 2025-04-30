#include "fill_init_convert.h"
#include "sycl/sycl.hpp"
#include "compute.h"

SYCL_EXTERNAL int getTrueIndexGrid(int width, int x, int y) {
	return y * width + x;
}

SYCL_EXTERNAL int myAbs(int input) {
    int mask = input >> 31;  // Vytvorí -1 pre záporné èísla, 0 pre kladné
    return (input + mask) ^ mask;
}

SYCL_EXTERNAL void writeMinimalPath(const MemoryPointers& globalMemory) {
	int minSize = globalMemory.agents[0].sizePath;;
	for (int i = 1; i < globalMemory.agentsCount; i++) {
		int candidate = globalMemory.agents[i].sizePath;
		if (candidate < minSize && candidate > 0) {
			minSize = candidate;
		}
	}
	if (minSize < 0){
		globalMemory.minSize_maxtimeStep_error[MINSIZE] = 0;
	}
	else {
		globalMemory.minSize_maxtimeStep_error[MINSIZE] = minSize;
	}
}

SYCL_EXTERNAL bool isSamePosition(Position& a, Position& b) {
	return bool(a.x == b.x && a.y == b.y);
}

SYCL_EXTERNAL int reconstructPath(int agentID, MemoryPointers& localMemory) {
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
		if ((index < 0 || index >= mapSize)) {
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

SYCL_EXTERNAL void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory) {
	localMemory.width = globalMemory.width;
	localMemory.height = globalMemory.height;
	localMemory.agentsCount = globalMemory.agentsCount;
	localMemory.loadersCount = globalMemory.loadersCount;
	localMemory.unloadersCount = globalMemory.unloadersCount;
	localMemory.grid = globalMemory.grid;
	localMemory.loaderPositions = globalMemory.loaderPositions;
	localMemory.unloaderPositions = globalMemory.unloaderPositions;
	localMemory.agents = globalMemory.agents;
	localMemory.minSize_maxtimeStep_error = globalMemory.minSize_maxtimeStep_error;
	// localMemory.numberConstraints = globalMemory.numberConstraints;

	const int mapSize = localMemory.width * localMemory.height;
	int offset = mapSize * agentId;
	localMemory.agentPaths = &(globalMemory.agentPaths[offset]);
	localMemory.gCost = &(globalMemory.gCost[offset]);
	localMemory.fCost = &(globalMemory.fCost[offset]);
	localMemory.cameFrom = &(globalMemory.cameFrom[offset]);
	localMemory.visited = &(globalMemory.visited[offset]);
	localMemory.openList = &(globalMemory.openList[offset]);
	// localMemory.constraints = &(globalMemory.constraints[offset / 4]);
}

SYCL_EXTERNAL void initializeCostsAndHeap(MemoryPointers& localMemory, positionHeap& myHeap, Position start) {
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
		ptr += offset;  // Posun na ïalšieho agenta
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
	CPUMemory.minSize_maxtimeStep_error[MINSIZE] = minSize;
}

int ManhattanHeuristic(const Position& a, const Position& b) {
	return myAbs(a.x - b.x) + myAbs(a.y - b.y);
}