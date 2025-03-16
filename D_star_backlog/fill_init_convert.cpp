#include "fill_init_convert.h"
#include "sycl/sycl.hpp"

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

int ManhattanHeuristic(const Position& a, const Position& b) {
	return myAbs(a.x - b.x) + myAbs(a.y - b.y);
}