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
#include "CBS.h"

namespace internal {
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
    void resolveConflictsCBS(AlgorithmType which, Map& m, CTNode& root, int numThreads, std::vector<std::vector<Position>>& solution) {
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
                    newNode1.paths[owner1] = ComputeCPUHIGHALGO(which, m, owner1, newNode1.constraints);
                    newNode2.paths[owner2] = ComputeCPUHIGHALGO(which, m, owner2, newNode2.constraints);
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
}