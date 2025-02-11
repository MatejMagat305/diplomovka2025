#include "compute.h"
#include <queue>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_set>

namespace internal {

    struct PositionOwner {
        Position pos1, pos2;
        int agentID1, agentID2;
        PositionOwner(Position p1, Position p2, int id1, int id2) : pos1(p1), pos2(p2), agentID1(id1), agentID2(id2) {}
        bool operator==(const PositionOwner& other) const {
            return pos2.x == other.pos2.x && pos2.y == other.pos2.y
                && pos1.x == other.pos1.x && pos1.y == other.pos1.y
                && agentID1 == other.agentID1 && agentID2 == other.agentID2;
        }
        bool operator!=(const PositionOwner& other) const {
            return !(*this == other);
        }
    };

    struct PositionOwnerHash {
        std::size_t operator()(const PositionOwner& p) const {
            std::size_t seed = 0;
            seed ^= std::hash<int>{}(p.agentID1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>{}(p.agentID2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>{}(p.pos1.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>{}(p.pos1.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>{}(p.pos2.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<int>{}(p.pos2.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };


    struct Constraint {
        Position pos;
        int agentID;
    };

    struct CTNode {
        std::vector<std::vector<Position>> paths;
        std::vector<std::vector<Constraint>> constraints;
        std::unordered_set<PositionOwner, PositionOwnerHash> conflicts;

        bool operator>(const CTNode& other) const {
            return (conflicts.size() > other.conflicts.size());
        }
    };

    struct Node {
        Position pos;
        int g, h, f;
        Node* parent;

        Node(Position pos, int g, int h, Node* parent = nullptr)
            : pos(pos), g(g), h(h), f(g + h), parent(parent) {
        }

        bool operator>(const Node& other) const {
            return f > other.f;
        }
    };
    inline bool isSamePosition(const Position& a, const Position& b) {
        return a.x == b.x && a.y == b.y;
    }

    int ManhattanHeuristic(const Position& a, const Position& b) {
        return abs(a.x - b.x) + abs(a.y - b.y);
    }

    Position findClosestLoader(const Agent& agent, const Map& m) {
        Position closestLoader = { -1, -1 };
        int minDistance = std::numeric_limits<int>::max();

        for (int i = 0; i < m.loaderCount; ++i) {
            Position loader = m.loaderPosition[i];
            int distance = ManhattanHeuristic(agent.position, loader);
            if (distance < minDistance) {
                minDistance = distance;
                closestLoader = loader;
            }
        }

        return closestLoader;
    }
    std::vector<Position> ComputeASTAR(Map& m, int agentID, const std::vector<Constraint>& constraints) {
        Agent& a = m.agents[agentID];
        Position start = a.position;
        Position goal = (a.direction == AGENT_LOADER)
            ? m.loaderPosition[a.loaderCurrent]
            : m.unloaderPosition[a.unloaderCurrent];

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::map<Position, int> costSoFar;
        std::map<Position, Node*> cameFrom;

        openSet.emplace(start, 0, ManhattanHeuristic(start, goal));
        costSoFar[start] = 0;

        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();

            if (isSamePosition(current.pos, goal)) {
                std::vector<Position> path;
                Node* node = &current;
                while (node) {
                    path.push_back(node->pos);
                    node = node->parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
            std::vector<Position> neighbors = {
                {current.pos.x + 1, current.pos.y},
                {current.pos.x - 1, current.pos.y},
                {current.pos.x, current.pos.y + 1},
                {current.pos.x, current.pos.y - 1}
            };

            for (const auto& next : neighbors) {
                if (next.x < 0 || next.y < 0 || next.x >= m.width || next.y >= m.height) continue;
                if (m.grid[next.y][next.x] == OBSTACLE_SYMBOL) continue;

                // Kontrola konfliktov v Constraint Tree
                bool conflict = false;
                for (const auto& constraint : constraints) {
                    if (constraint.agentID == agentID && isSamePosition(constraint.pos, next)) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;

                int newCost = costSoFar[current.pos] + 1;
                if (!costSoFar.count(next) || newCost < costSoFar[next]) {
                    costSoFar[next] = newCost;
                    Node* newNode = new Node(next, newCost, ManhattanHeuristic(next, goal), new Node(current.pos, current.g, current.h, current.parent));
                    cameFrom[next] = newNode;
                    openSet.push(*newNode);
                }
            }
        }

        return {}; // Ak nie je možné nájs cestu
    }

    std::vector<Position> ComputeshortestPathALGO(AlgorithmType which, Map& m, int agentID, const std::vector<Constraint>& constraints) {
        switch (which) {
        case AlgorithmType::ASTAR:
            return ComputeASTAR(m, agentID, constraints);
        default:
            return ComputeASTAR(m, agentID, constraints);
        }
    }

    void reDetectConflicts(CTNode& node, const std::vector<int>& changedAgentIndices) {
        for (int i : changedAgentIndices) {
            for (size_t j = 0; j < node.paths.size(); j++) {
                if (i == j) continue;
                size_t minS = std::min(node.paths[i].size(), node.paths[j].size());
                for (size_t k = 0; k < minS; k++) {
                    PositionOwner tupple = PositionOwner(node.paths[i][k], node.paths[j][k], i, j);
                    if (isSamePosition( node.paths[i][k], node.paths[j][k])) {
                        node.conflicts.insert(tupple);
                        break;
                    }
                    if (k > 0 
                        && isSamePosition( node.paths[i][k], node.paths[j][k - 1]) 
                        && isSamePosition( node.paths[j][k], node.paths[i][k - 1])) {
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
                    if (isSamePosition( node.paths[i][k], node.paths[j][k])) {
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

    void resolveConflictsCBS_CPU(AlgorithmType which, Map& m, int numThreads) {
        std::priority_queue<CTNode, std::vector<CTNode>, std::greater<CTNode>> openSet;
        std::mutex openSetMutex;
        std::condition_variable openSetCV;
        std::vector<std::vector<Position>> solution;
        std::atomic<bool> solutionFound = false;

        // Vytvorenie koreòového uzla
        CTNode root;
        for (int i = 0; i < m.agentCount; i++) {
            root.paths.push_back(internal::ComputeshortestPathALGO(which, m, i, {}));
        }
        detectConflicts(root);
        openSet.push(root);
        std::vector<std::future<void>> workers;
        auto worker = [&]() {
            while (!solutionFound) {
                CTNode current;
                {
                    std::unique_lock<std::mutex> lock(openSetMutex);
                    openSetCV.wait(lock, [&] { return !openSet.empty() || solutionFound; });
                    if (solutionFound) return;
                    if (openSet.empty()) continue;
                    current = openSet.top();
                    openSet.pop();
                }
                if (current.conflicts.empty()) {
                    std::lock_guard<std::mutex> lock(openSetMutex);
                    if (!solutionFound.exchange(true)) {
                        solution = current.paths;
                    }
                    openSetCV.notify_all();
                    return;
                }
                for (auto& conflictOwner : current.conflicts) {
                    Position conflictPos1 = conflictOwner.pos1, conflictPos2 = conflictOwner.pos2;
                    int owner1 = conflictOwner.agentID1, owner2 = conflictOwner.agentID2;
                    CTNode newNode1 = current;
                    CTNode newNode2 = current;
                    newNode1.constraints[owner1].push_back({ conflictPos1 });
                    newNode2.constraints[owner2].push_back({ conflictPos2 });
                    newNode1.conflicts.erase(conflictOwner);
                    newNode2.conflicts.erase(conflictOwner);
                    newNode1.paths[owner1] = ComputeshortestPathALGO(which, m, owner1, newNode1.constraints[owner1]);
                    newNode2.paths[owner2] = ComputeshortestPathALGO(which, m, owner2, newNode2.constraints[owner2]);
                    reDetectConflicts(newNode1, { owner1 });
                    reDetectConflicts(newNode2, { owner2 });
                    {
                        std::lock_guard<std::mutex> lock(openSetMutex);
                        openSet.push(newNode1);
                        openSet.push(newNode2);
                    }
                    openSetCV.notify_all();
                }
            }
            };

        for (int i = 0; i < numThreads; i++) {
            workers.push_back(std::async(std::launch::async, worker));
        }

        for (auto& worker : workers) {
            worker.get();
        }
        Position** Paths = m.agentPaths;
        for (int i = 0; i < solution.size(); i++){
            if (Paths[i] != nullptr) {
                delete[] Paths[i];
            }
            Position* Path = Paths[i];
            std::vector<Position>& solutionPath = solution[i];
            int sizePath = solutionPath.size();
            Paths[i] = new Position[sizePath];
            m.agentPathSizes[i] = sizePath;
            for (int j = 0; j < sizePath; j++){ 
                Position& sP = solutionPath[j];
                Position& p = Path[j];
                p.x = sP.x;
                p.y = sP.y;
            }
        }
    }
}

double computeCPU(AlgorithmType which, Map& m, int numThreads) {
    auto start_time = std::chrono::high_resolution_clock::now();
    internal::resolveConflictsCBS_CPU(which, m, numThreads);
    auto end_time = std::chrono::high_resolution_clock::now();
    internal::setGPUSynch(false);
    return std::chrono::duration<double>(end_time - start_time).count();
}

