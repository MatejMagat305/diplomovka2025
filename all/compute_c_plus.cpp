#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include "fill_init_convert.h"
#include "compute.h"
#include "d_star_algo.h"
#include <chrono>
#include <utility>
#include <algorithm>

std::vector<std::priority_queue<std::pair<double, Position>,
	std::vector<std::pair<double, Position>>,
	std::greater<>>> openSets;


std::vector<Position> getNeighbors(const Position& u, const Map& m) {
	std::vector<Position> neighbors;
	Position possibleMoves[4] = {
		{u.x + 1, u.y}, {u.x - 1, u.y},
		{u.x, u.y + 1}, {u.x, u.y - 1}
	};
	for (int i = 0; i < 4; i++) {
		Position s = possibleMoves[i];
		if (s.x < 0 || s.x >= m.CPUMemory.width || s.y < 0 || s.y >= m.CPUMemory.height) {
			continue;
		}
		if (m.CPUMemory.grid[getTrueIndexGrid(m.CPUMemory.width, s.x, s.y)] == OBSTACLE_SYMBOL) {
			continue;
		}
		neighbors.push_back(s);
	}
	return neighbors;
}

void updateVertex(Node& node, std::unordered_map<Position, Node>& nodes, Position goal, const Map& m, int agentID) {
	if (!isSamePosition(goal, node.pos)) {
		node.rhs = std::numeric_limits<double>::infinity();
		for (auto& neighbor : getNeighbors(node.pos, m)) {
			if (nodes.find(neighbor) != nodes.end()) {
				double temp = (double)(nodes[neighbor].g + 1.0);
				node.rhs = std::min((double)(node.rhs), temp);
			}
		}
	}
	double priority = std::min(node.g, node.rhs);
	if (priority != std::numeric_limits<double>::infinity()) {
		priority += ManhattanHeuristic(node.pos, goal);
		openSets[agentID].emplace(priority, node.pos);
	}
}

std::vector<Position> ComputeDSTAR(Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
    Agent& a = m.CPUMemory.agents[agentID];
    Position start = {a.x, a.y};
    Position goal = (a.direction == AGENT_LOADER)? 
        m.CPUMemory.loaderPositions[a.loaderCurrent] :
        m.CPUMemory.unloaderPositions[a.unloaderCurrent];
    std::unordered_map<Position, Node> nodes;
    for (int x = 0; x < m.CPUMemory.width; x++) {
        for (int y = 0; y < m.CPUMemory.height; y++) {
            nodes[{x, y}] = Node{ {x, y}, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), 0 };
        }
    }
    nodes[goal].rhs = 0;
    openSets[agentID].emplace(0, goal);
    while (!openSets[agentID].empty()) {
        Position pos = openSets[agentID].top().second;
        openSets[agentID].pop();
        if (isSamePosition(pos, start)) break;
        Node& current = nodes[pos];
        if (current.g > current.rhs) {
            current.g = current.rhs;
            for (auto& neighbor : getNeighbors(pos, m)) {
                if (nodes.find(neighbor) != nodes.end()) {
                    updateVertex(nodes[neighbor], nodes, goal, m, agentID);
                }
            }
        }
        else {
            current.g = std::numeric_limits<double>::infinity();
            updateVertex(current, nodes, goal, m, agentID);
            for (auto& neighbor : getNeighbors(pos, m)) {
                updateVertex(nodes[neighbor], nodes, goal, m, agentID);
            }
        }
    }
    std::vector<Position> path;
    Position pos = start;
    while (! isSamePosition(pos, goal)) {
        path.push_back(pos);
		auto neighbors = getNeighbors(pos, m);
		auto it = std::min_element(neighbors.begin(), neighbors.end(), [&](const Position& a, const Position& b) {
			return nodes[a].g < nodes[b].g;
			});
		if (it != neighbors.end()) pos = *it;
    }
	return (!path.empty() && isSamePosition(path.back(), goal)) ? path : std::vector<Position>();
}

std::vector<Position> ComputeCPUHIGHALGO(AlgorithmType which, Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
    switch (which) {
    case AlgorithmType::DSTAR:
        return ComputeDSTAR(m, agentID, constraints);
    default:
        return ComputeDSTAR(m, agentID, constraints);
    }
}

void computeInitialPaths(AlgorithmType which, Map& m, CTNode& root) {
	root.paths.resize(m.CPUMemory.agentsCount);
	for (int agentId = 0; agentId < m.CPUMemory.agentsCount; agentId++) {
		int sizePath = m.CPUMemory.agents[agentId].sizePath;
		if (sizePath > 0) {
			Position* path = &(m.CPUMemory.agentPaths[agentId * m.CPUMemory.height * m.CPUMemory.width]);
			root.paths[agentId] = std::vector<Position>(path, path + sizePath - 1);
			continue;
		}
		std::vector<std::vector<Constrait>> x(m.CPUMemory.agentsCount, std::vector<Constrait>());
		root.paths[agentId] = ComputeCPUHIGHALGO(which, std::ref(m), agentId, x);
	}
}

void reDetectConflicts(CTNode& node, const std::vector<int>& changedAgentIndices) {
	for (int i : changedAgentIndices) {
		for (size_t j = 0; j < node.paths.size(); j++) {
			if (i == j) continue;
			size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
			for (size_t k = 0; k < minS; k++) {
				PositionOwner tupple = PositionOwner(node.paths[i][k], node.paths[j][k], i, j);
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
					node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j));
					break;
				}
				if (k > 0 && isSamePosition(node.paths[i][k], node.paths[j][k - 1]) && isSamePosition(node.paths[j][k], node.paths[i][k - 1])) {
					node.conflicts.insert(PositionOwner(node.paths[i][k], node.paths[j][k], i, j));
					break;
				}
			}
		}
	}
}

void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, std::vector<std::vector<Position>>& solution) {
	std::priority_queue<CTNode, std::vector<CTNode>, std::greater<CTNode>> openSet;
	detectConflicts(root);
	openSet.push(root);
	while (!openSet.empty()) {
		CTNode current = openSet.top();
		openSet.pop();
		if (current.conflicts.empty()) {
			solution = current.paths;
			return;
		}
		for (auto& conflictOwner : current.conflicts) {
			Position conflictPos1 = conflictOwner.pos1, conflictPos2 = conflictOwner.pos2;
			int owner1 = conflictOwner.agentID1, owner2 = conflictOwner.agentID2;
			CTNode newNode1 = current;
			CTNode newNode2 = current;
			Constrait c1, c2;
			c1.to.x = conflictPos1.x;
			c1.to.y = conflictPos1.y;
			c2.to.x = conflictPos2.x;
			c2.to.y = conflictPos2.y;
			newNode1.constraints[owner1].push_back(c1);
			newNode2.constraints[owner2].push_back(c2);
			newNode1.conflicts.erase(conflictOwner);
			newNode2.conflicts.erase(conflictOwner);
			newNode1.paths[owner1] = ComputeCPUHIGHALGO(which, m, owner1, newNode1.constraints);
			newNode2.paths[owner2] = ComputeCPUHIGHALGO(which, m, owner2, newNode2.constraints);
			reDetectConflicts(newNode1, { owner1 });
			reDetectConflicts(newNode2, { owner2 });
			openSet.push(newNode1);
			openSet.push(newNode2);
		}
	}
	solution = root.paths;
}

Info computeCPU(AlgorithmType which, Map& m) {
	openSets.clear();
	openSets.resize(m.CPUMemory.agentsCount);
	auto start_time = std::chrono::high_resolution_clock::now();
	CTNode root;
	computeInitialPaths(which, m, root);
	std::vector<std::vector<Position>> solution;
	resolveConflictsCBS(which, m, root, solution);
	auto end_time = std::chrono::high_resolution_clock::now();
	pushVector(solution, m);
	auto copy_time = std::chrono::high_resolution_clock::now();
	Info result{ 0,0 };
	result.timeRun = std::chrono::duration<double>(end_time - start_time).count();
	result.timeSynchronize = std::chrono::duration<double>(copy_time - end_time).count();
	return result;
}