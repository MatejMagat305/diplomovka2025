#include "fill_init_convert.h"

int getTrueIndexGrid(int width, int x, int y) {
	return y * width + x;
}

int myAbs(int input) {
	if (input < 0) {
		return -input;
	}
	return input;
}

void fillLocalMemory(const MemoryPointers& globalMemory, int agentId, MemoryPointers& localMemory) {
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

void initializeCostsAndHeap(MemoryPointers& localMemory, MyHeap& myHeap, Position start) {
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

std::vector<std::vector<Position>> getVector(Map& m) {
	size_t offset = m.CPUMemory.width * m.CPUMemory.height;
	std::vector<std::vector<Position>> result(m.CPUMemory.agentCount);

	Position* ptr = m.CPUMemory.pathsAgent;
	for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
		size_t pathSize = m.CPUMemory.agents[i].sizePath;
		result[i].assign(ptr, ptr + pathSize);  // Skopírujeme dáta do vektora
		ptr += offset;  // Posun na ïalšieho agenta
	}
	return result;
}


void pushVector(const std::vector<std::vector<Position>>& vec2D, Map& m) {
	size_t offset = m.CPUMemory.width * m.CPUMemory.height;
	Position* ptr = m.CPUMemory.pathsAgent;
	int minSize = vec2D[0].size();
	for (int i = 0; i < m.CPUMemory.agentCount; ++i) {
		const std::vector<Position>& row = vec2D[i];
		std::copy(row.begin(), row.end(), ptr);
		ptr += offset;
		int agentSizePath = row.size();
		m.CPUMemory.agents[i].sizePath = agentSizePath;
		if (agentSizePath < minSize) {
			minSize = agentSizePath;
		}
	}
	*m.CPUMemory.minSize = minSize;
}

bool isSamePosition(const Position& a, const Position& b) {
	return a.x == b.x && a.y == b.y;
}

int ManhattanHeuristic(const Position& a, const Position& b) {
	return myAbs(a.x - b.x) + myAbs(a.y - b.y);
}

