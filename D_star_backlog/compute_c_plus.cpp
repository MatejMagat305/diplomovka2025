#include <vector>
#include <queue>
#include <unordered_set>
#include <limits>
#include <chrono>
#include <algorithm>
#include "fill_init_convert.h"
#include "compute.h"
#include "d_star_algo.h"


std::vector<Position> getNeighbors(const Position& u, const Map& m) {
    std::vector<Position> neighbors;
    Position possibleMoves[4] = {
    {u.x + 1, u.y}, {u.x - 1, u.y},
    {u.x, u.y + 1}, {u.x, u.y - 1} };
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

DStarState initializeDStar(const Map& m, Position goal) {
    DStarState state;
    for (int x = 0; x < m.CPUMemory.width; x++) {
        for (int y = 0; y < m.CPUMemory.height; y++) {
            state.nodes[{x, y}] = Node{ {x, y}, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), 0 };
        }
    }
    state.nodes[goal].rhs = 0;
    state.openSet.emplace(0, goal);
    return state;
}

void updateVertex(Node& node, DStarState& state, Position goal, const Map& m) {
    if (!isSamePosition(goal, node.pos)) {
        node.rhs = std::numeric_limits<double>::infinity();
        for (auto& neighbor : getNeighbors(node.pos, m)) {
            if (state.nodes.find(neighbor) != state.nodes.end()) {
                double temp = state.nodes[neighbor].g + 1.0;
                node.rhs = std::min(node.rhs, temp);
            }
        }
    }
    double priority = std::min(node.g, node.rhs);
    if (priority != std::numeric_limits<double>::infinity()) {
        priority += ManhattanHeuristic(node.pos, goal);
        state.openSet.emplace(priority, node.pos);
        state.visited.insert(node.pos);
    }
}

void updateDStarConstraints(DStarState& state, const std::vector<Constraint>& constraints, const Map& m) {
    for (const auto& constraint : constraints) {
        state.nodes[constraint.to].g = std::numeric_limits<double>::infinity();
        state.nodes[constraint.to].rhs = std::numeric_limits<double>::infinity();
        for (auto& neighbor : getNeighbors(constraint.to, m)) {
            updateVertex(state.nodes[neighbor], state, constraint.to, m);
        }
    }
}

std::vector<Position> ComputeDSTAR(Map& m, const std::vector<Constraint>& constraints, DStarState& state, Position start, Position goal) {
    updateDStarConstraints(state, constraints, m);

    while (!state.openSet.empty()) {
        auto [_, pos0] = state.openSet.top();
        state.openSet.pop();

        if (isSamePosition(pos0, start)) break;

        Node& current = state.nodes[pos0];
        if (current.g > current.rhs) {
            current.g = current.rhs;
            for (auto& neighbor : getNeighbors(pos0, m)) {
                updateVertex(state.nodes[neighbor], state, goal, m);
            }
        }
        else {
            current.g = std::numeric_limits<double>::infinity();
            updateVertex(current, state, goal, m);
            for (auto& neighbor : getNeighbors(pos0, m)) {
                updateVertex(state.nodes[neighbor], state, goal, m);
            }
        }
    }

    std::vector<Position> path;
    Position pos = start;
    while (!isSamePosition(pos, goal)) {
        path.push_back(pos);
        auto neighbors = getNeighbors(pos, m);
        auto it = std::min_element(neighbors.begin(), neighbors.end(), [&](const Position& a, const Position& b) {
            return state.nodes[a].g < state.nodes[b].g;
            });
        if (it == neighbors.end() || state.nodes[*it].g == std::numeric_limits<double>::infinity()) break;
        pos = *it;
    }
    path.push_back(goal);
    return path;
}

Info computeCPU(AlgorithmType which, Map& m) {
    auto start_time = std::chrono::high_resolution_clock::now();
    CTNode root;
    root.paths.resize(m.CPUMemory.agentsCount);
    std::vector<DStarState> dstarStates(m.CPUMemory.agentsCount);

    for (int agentId = 0; agentId < m.CPUMemory.agentsCount; agentId++) {
        Agent& a = m.CPUMemory.agents[agentId];
        Position start = { a.x, a.y };
        Position goal = (a.direction == AGENT_LOADER) ?
            m.CPUMemory.loaderPositions[a.loaderCurrent] :
            m.CPUMemory.unloaderPositions[a.unloaderCurrent];

        if (dstarStates[agentId].nodes.empty()) {
            dstarStates[agentId] = initializeDStar(m, goal);
        }

        root.paths[agentId] = ComputeDSTAR(m, {}, dstarStates[agentId], start, goal);
    }

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





struct CompareNodes {
    bool operator()(const CTNode& a, const CTNode& b) const {
        return a > b;
    }
};

void detectConflicts(CTNode& node) {
    node.conflicts.clear();
    for (size_t i = 0; i < node.paths.size(); i++) {
        for (size_t j = i + 1; j < node.paths.size(); j++) {
            for (size_t t = 0; t < std::min(node.paths[i].size(), node.paths[j].size()); t++) {
                PositionOwner conflict(node.paths[i][t], node.paths[j][t], static_cast<int>(i), static_cast<int>(j));
                if (node.paths[i][t] == node.paths[j][t]) {
                    node.conflicts.insert(conflict);
                }
                if (t > 0 && node.paths[i][t] == node.paths[j][t - 1] && node.paths[i][t - 1] == node.paths[j][t]) {
                    node.conflicts.insert(conflict);
                }
            }
        }
    }
}

void reDetectConflicts(CTNode& node, const std::vector<int>& agentsToCheck) {
    for (int agent : agentsToCheck) {
        for (size_t j = 0; j < node.paths.size(); j++) {
            if (agent == static_cast<int>(j)) continue;
            for (size_t t = 0; t < std::min(node.paths[agent].size(), node.paths[j].size()); t++) {
                PositionOwner conflict(node.paths[agent][t], node.paths[j][t], agent, static_cast<int>(j));
                if (node.paths[agent][t] == node.paths[j][t]) {
                    node.conflicts.insert(conflict);
                }
                else if (t > 0 && node.paths[agent][t] == node.paths[j][t - 1] && node.paths[agent][t - 1] == node.paths[j][t]) {
                    node.conflicts.insert(conflict);
                }
                else {
                    node.conflicts.erase(conflict);
                }
            }
        }
    }
}

void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, std::vector<std::vector<Position>>& solution) {
    std::priority_queue<CTNode, std::vector<CTNode>, CompareNodes> openList;
    openList.push(root);

    while (!openList.empty()) {
        CTNode currentNode = openList.top();
        openList.pop();

        detectConflicts(currentNode);
        if (currentNode.conflicts.empty()) {
            solution = currentNode.paths;
            return;
        }

        auto conflict = *(currentNode.conflicts.begin());

        for (int i = 0; i < 2; i++) {
            int agent = (i == 0) ? conflict.agentID1 : conflict.agentID2;
            Position pos = (i == 0) ? conflict.pos1 : conflict.pos2;

            CTNode newNode = currentNode;
            Constraint newConstraint{ pos, agent };
            newNode.constraints[agent].push_back(newConstraint);

            DStarState dstarState = initializeDStar(m, newNode.paths[agent].back());
            Position start = { m.CPUMemory.agents[agent].x, m.CPUMemory.agents[agent].y };
            newNode.paths[agent] = ComputeDSTAR(m, newNode.constraints[agent], dstarState, start, newNode.paths[agent].back());
            reDetectConflicts(newNode, { agent });

            openList.push(newNode);
        }
    }
}
