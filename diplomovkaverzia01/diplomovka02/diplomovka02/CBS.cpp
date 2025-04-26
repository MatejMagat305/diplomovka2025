#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include "fill_init_convert.h"
#include "a_star_algo.h"
#include "compute.h"
#include "solve_conflicts.h" 

void reDetectConflicts(CTNode& node, const std::vector<int>& changedAgentIndices) {
	for (int i : changedAgentIndices) {
		for (size_t j = 0; j < node.paths.size(); j++) {
			if (i == j) continue;
			size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
			for (size_t k = 0; k < minS; k++) {
				PositionOwner tupple = PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k);
				if (isSamePosition(node.paths[i][k], node.paths[j][k])) {
					node.conflicts.insert(tupple);
					break;
				}
				if (k > 0
					&& isSamePosition(node.paths[i][k], node.paths[j][k - 1])
					&& isSamePosition(node.paths[j][k], node.paths[i][k - 1])) {
					node.conflicts.insert(tupple);
					break;
				}
				node.conflicts.erase(tupple);
			}
		}
	}
}

void detectConflicts(CTNode& node) {
	node.conflicts.clear();
	for (size_t i = 0; i < node.paths.size(); i++) {
		for (size_t j = i + 1; j < node.paths.size(); j++) {
			size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
			for (size_t k = 0; k < minS; k++) {
				if (isSamePosition(node.paths[i][k], node.paths[j][k])) {
					node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k));
					break;
				}
				if (k > 0 && isSamePosition(node.paths[i][k], node.paths[j][k - 1]) && isSamePosition(node.paths[j][k], node.paths[i][k - 1])) {
					node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j, k));
					break;
				}
			}
		}
	}
}

std::vector<Position> findFreeBlockPark(Map *m, int ID, const CTNode& node, int coliderID ) {
	MemoryPointers& CPUMemory = m->CPUMemory;
	int width = CPUMemory.width, height = CPUMemory.height;
	std::vector<Position> path = node.paths[coliderID];
	int maxSize = node.paths[coliderID].size();

	for (int i = 0; i < maxSize; i++) {
		Position& current = path[i];
		Position& prew = path[i];
		if (i>0){
			prew = path[i - 1];
		}
		std::vector<Position> neighbors = {
			{current.x + 1, current.y},
			{current.x - 1, current.y},
			{current.x, current.y + 1},
			{current.x, current.y - 1}
		};
		for (auto& next : neighbors) {
			if (next.x < 0 || next.y < 0 || next.x >= width || next.y >= height) continue;
			if (CPUMemory.grid[getTrueIndexGrid(width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;
			if (isSamePosition(next, prew)) continue;
			bool blocked = false;
			for (int i = 0; i < node.constraints[ID].size(); i++) {
				Constraint c = node.constraints[ID][i];
				if (c.position.x == next.x && c.position.y == next.y) {
					blocked = true;
					break;
				}
			}
			if (blocked) continue;
			CPUMemory.agents[ID].goal = next;
			if (CPUMemory.agents[ID].direction == AGENT_LOADER){
				CPUMemory.agents[ID].direction = AGENT_PARK_LOADER;
			}
			else if(CPUMemory.agents[ID].direction == AGENT_UNLOADER){
				CPUMemory.agents[ID].direction = AGENT_PARK_UNLOADER;
			}
			auto path = ComputeASTAR(m, ID, node.constraints);
			if (!path.empty()) {
				CPUMemory.agents[ID].sizePath = path.size();
				return path;
			}
		}
	}
	return {};

}

std::vector<std::vector<Position>> resolveConflictsCBS(Map* m, CTNode& root) {
	std::priority_queue<CTNode, std::vector<CTNode>, std::greater<CTNode>> openSet;
	detectConflicts(root);
	openSet.push(root);
	while (!openSet.empty()) {
		CTNode current;
		current = openSet.top();
		openSet.pop();
		if (current.conflicts.empty()) {
			return current.paths;
		}
		for (auto& conflictOwner : current.conflicts) {
			Position conflictPos1 = conflictOwner.pos1, conflictPos2 = conflictOwner.pos2;
			int owner1 = conflictOwner.agentID1, owner2 = conflictOwner.agentID2;
			CTNode newNode1 = current;
			CTNode newNode2 = current;
			Constraint c1, c2;
			c1.position = conflictPos1;
			c2.position = conflictPos2;
			newNode1.constraints[owner1].push_back(c1);
			newNode1.conflicts.erase(conflictOwner);
			newNode2.constraints[owner2].push_back(c2);
			newNode2.conflicts.erase(conflictOwner);
			if (shouldYield(owner2, owner2, m->CPUMemory.agents)) {
				auto path1 = ComputeASTAR(m, owner1, newNode1.constraints);
				if (path1.empty()) {
					path1 = findFreeBlockPark(m, owner1, newNode1, owner2);
				}
				if (!path1.empty()) {
					newNode1.paths[owner1] = path1;
					reDetectConflicts(newNode1, { owner1 });
					openSet.push(newNode1);
				}
			}
			else {
				auto path2 = ComputeASTAR(m, owner2, newNode2.constraints);
				if (path2.empty()) {
					path2 = findFreeBlockPark(m, owner2, newNode2, owner1);
				}
				if (!path2.empty()) {
					newNode2.paths[owner2] = path2;
					reDetectConflicts(newNode2, { owner2 });
					openSet.push(newNode2);
				}
			}
		}
	}
	return {};
}