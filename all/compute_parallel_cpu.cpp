#include "compute.h"
#include <queue>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include <chrono>
#include "heap_primitive.h"
#include "fill_init_convert.h"

namespace internal {

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
            Position loader = m.CPUMemory.loaderPosition[i];
            int distance = ManhattanHeuristic(Position{agent.x, agent.y}, loader);
            if (distance < minDistance) {
                minDistance = distance;
                closestLoader = loader;
            }
        }

        return closestLoader;
    }
    std::vector<Position> ComputeASTAR(Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
        Agent& a = m.CPUMemory.agents[agentID];
        Position start = Position{a.x, a.y};
        Position goal = (a.direction == AGENT_LOADER)
            ? m.CPUMemory.loaderPosition[a.loaderCurrent]
            : m.CPUMemory.unloaderPosition[a.unloaderCurrent];

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
                if (m.CPUMemory.grid[getTrueIndexGrid(m.width, next.x, next.y)] == OBSTACLE_SYMBOL) continue;

                // Kontrola konfliktov v Constraint Tree
                bool conflict = false;
                auto constraintsAgent = constraints[agentID];
                for (const auto& constraint : constraintsAgent) {
                    Position actual{};
                    actual.x = constraint.x;
                    actual.y = constraint.y;
                    if (isSamePosition(actual, next)) {
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

    std::vector<Position> ComputeshortestPathALGO(AlgorithmType which, Map& m, int agentID, const std::vector<std::vector<Constrait>>& constraints) {
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

    // 1. Funkcia pre výpoèet poèiatoèných ciest pre agentov
    void computeInitialPaths(AlgorithmType which, Map& m, CTNode& root, int numThreads) {
        root.paths.resize(m.agentCount);
        std::vector<std::thread> threads;
        std::mutex mtx;
        std::vector<bool> processedAgents(m.agentCount, false);

        for (int i = 0; i < numThreads; i++) {
            threads.push_back(std::thread([&]() {
                while (true) {
                    int agentId = 0;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        for (; agentId < m.agentCount; agentId++) {
                            if (!processedAgents[agentId]) {
                                break;
                            }
                        }
                        if (agentId == m.agentCount) break;
                        processedAgents[agentId] = true;
                    }

                    std::vector<Position> path = ComputeshortestPathALGO(which, std::ref(m), agentId, const std::vector<std::vector<Constrait>>{});
                    root.paths[agentId] = path;
                }
                }));
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    // 2. Funkcia pre riešenie konfliktov pomocou CBS
    void resolveConflictsCBS(AlgorithmType which, Map & m, CTNode & root, int numThreads, std::vector<std::vector<Position>>&solution) {
            std::priority_queue<CTNode, std::vector<CTNode>, std::greater<CTNode>> openSet;
            std::mutex openSetMutex;
            std::condition_variable openSetCV;
            std::atomic<bool> solutionFound = false;

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
                        Constrait c1, c2;
                        c1.x = conflictPos1.x;
                        c1.y = conflictPos1.y;
                        c2.x = conflictPos2.x;
                        c2.y = conflictPos2.y;
                        newNode1.constraints[owner1].push_back(c1);
                        newNode2.constraints[owner2].push_back(c2);
                        newNode1.conflicts.erase(conflictOwner);
                        newNode2.conflicts.erase(conflictOwner);
                        newNode1.paths[owner1] = ComputeshortestPathALGO(which, m, owner1, newNode1.constraints);
                        newNode2.paths[owner2] = ComputeshortestPathALGO(which, m, owner2, newNode2.constraints);
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
            solution = root.paths;
        }


    // 3. Funkcia, ktorá spája obe predchádzajúce a spracuje výsledok
    void resolveConflictsCBS_CPU(AlgorithmType which, Map& m, int numThreads) {
        CTNode root;
        computeInitialPaths(which, m, root, numThreads);
        std::vector<std::vector<Position>> solution;
        resolveConflictsCBS(which, m, root, numThreads, solution);

        // Spracovanie výslednej cesty (rovnaké ako predtým)
        Position* Paths = m.CPUMemory.pathsAgent;
        int offset = m.agentCount*m.height*
        for (int i = 0; i < m.agentCount; i++) {
            Position* Path = Paths[i];
            std::vector<Position>& solutionPath = solution[i];
            int sizePath = solutionPath.size();
            Paths[i] = new Position[sizePath];
            m.agentPathSizes[i] = sizePath;
            for (int j = 0; j < sizePath; j++) {
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

